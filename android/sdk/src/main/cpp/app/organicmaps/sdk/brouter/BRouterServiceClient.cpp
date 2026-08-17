#include "app/organicmaps/sdk/brouter/BRouterServiceClient.hpp"

#include "app/organicmaps/sdk/core/jni_helper.hpp"
#include "app/organicmaps/sdk/core/ScopedEnv.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <string>
#include <vector>

// libdeflate is bundled with Android NDK; use it for gzip decoding.
#include <zlib.h>

namespace
{
// jclass / jmethodID resolved via the application's classloader. FindClass is
// not enough when the current thread was attached via AttachCurrentThread
// because the calling thread's classloader is the bootstrap one, not the
// app's. Use Class.forName(name, true, classLoader) instead.
jclass g_contextClass = nullptr;
jmethodID g_getClassLoaderId = nullptr;
jclass g_classClass = nullptr;
jmethodID g_forNameId = nullptr;
jclass g_appContextClass = nullptr;
jmethodID g_currentApplicationId = nullptr;
jobject g_classLoader = nullptr;

jclass GetAppClassRef(JNIEnv * env, char const * sig)
{
  if (g_contextClass == nullptr)
    g_contextClass = jni::GetGlobalClassRef(env, "android/content/Context");
  if (g_getClassLoaderId == nullptr)
    g_getClassLoaderId = env->GetMethodID(g_contextClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
  if (g_classClass == nullptr)
    g_classClass = jni::GetGlobalClassRef(env, "java/lang/Class");
  if (g_forNameId == nullptr)
    g_forNameId = env->GetStaticMethodID(g_classClass, "forName",
                                        "(Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;");
  if (g_appContextClass == nullptr)
    g_appContextClass = jni::GetGlobalClassRef(env, "android/app/ActivityThread");
  if (g_currentApplicationId == nullptr)
    g_currentApplicationId = env->GetStaticMethodID(g_appContextClass, "currentApplication",
                                                    "()Landroid/app/Application;");

  jobject app = env->CallStaticObjectMethod(g_appContextClass, g_currentApplicationId);
  if (jni::HandleJavaException(env))
  {
    LOG(LWARNING, ("BRouterServiceClient: failed to obtain Application context"));
    return nullptr;
  }
  jni::ScopedLocalRef<jobject> const appRef(env, app);

  if (g_classLoader == nullptr)
  {
    jobject cl = env->CallObjectMethod(appRef.get(), g_getClassLoaderId);
    if (jni::HandleJavaException(env) || cl == nullptr)
    {
      LOG(LWARNING, ("BRouterServiceClient: failed to obtain classloader"));
      return nullptr;
    }
    g_classLoader = env->NewGlobalRef(cl);
    env->DeleteLocalRef(cl);
  }

  jni::ScopedLocalRef<jstring> const sigRef(env, env->NewStringUTF(sig));
  jclass klass = static_cast<jclass>(
      env->CallStaticObjectMethod(g_classClass, g_forNameId, sigRef.get(), JNI_TRUE, g_classLoader));
  if (jni::HandleJavaException(env))
  {
    LOG(LWARNING, ("BRouterServiceClient: Class.forName threw for ", sig, " classLoader=",
                   (void *)g_classLoader));
    return nullptr;
  }
  if (klass == nullptr)
  {
    LOG(LWARNING, ("BRouterServiceClient: Class.forName returned null for ", sig));
    return nullptr;
  }
  return static_cast<jclass>(env->NewGlobalRef(klass));
}

jobject GetApplicationContext(JNIEnv * env)
{
  if (g_appContextClass == nullptr || g_currentApplicationId == nullptr)
  {
    if (GetAppClassRef(env, "android/app/ActivityThread") == nullptr)
      return nullptr;
  }
  jobject app = env->CallStaticObjectMethod(g_appContextClass, g_currentApplicationId);
  if (jni::HandleJavaException(env))
    return nullptr;
  return app;
}

// Decode the ejY0 (base64 + gzip) envelope that BRouter emits when
// `acceptCompressedResult=true`. Returns true on success.
bool DecodeCompressed(std::string const & in, std::string & out)
{
  // Prefix "ejY0" is base64 for "z64".
  if (in.size() < 4 || in.compare(0, 4, "ejY0") != 0)
    return false;

  // android.util.Base64 lives on the platform classpath, not the app
  // classpath. Resolve it with FindClass (which walks the boot loader)
  // rather than Class.forName + app class loader.
  ScopedEnv env(jni::GetJVM());
  if (!env)
    return false;

  static jclass g_base64Class = nullptr;
  static jmethodID g_decodeMethod = nullptr;
  if (g_base64Class == nullptr)
  {
    g_base64Class = jni::GetGlobalClassRef(env.get(), "android/util/Base64");
    if (g_base64Class == nullptr)
      return false;
  }
  if (g_decodeMethod == nullptr)
  {
    g_decodeMethod = env->GetStaticMethodID(g_base64Class, "decode",
                                           "(Ljava/lang/String;I)[B");
    if (g_decodeMethod == nullptr)
      return false;
  }

  jni::ScopedLocalRef<jstring> const jStr(env.get(), env->NewStringUTF(in.c_str()));
  jni::ScopedLocalRef<jbyteArray> const jBytes(
      env.get(), static_cast<jbyteArray>(env->CallStaticObjectMethod(
                     g_base64Class, g_decodeMethod, jStr.get(), 0 /* DEFAULT */)));
  if (jni::HandleJavaException(env.get()) || !jBytes.get())
    return false;

  jsize const len = env->GetArrayLength(jBytes.get());
  std::vector<unsigned char> buf(len);
  env->GetByteArrayRegion(jBytes.get(), 0, len, reinterpret_cast<jbyte *>(buf.data()));

  // Skip the 3-byte "z64" magic.
  if (len < 3 || buf[0] != 'z' || buf[1] != '6' || buf[2] != '4')
    return false;

  // Inflate gzip-compressed bytes.
  z_stream strm{};
  strm.next_in = buf.data() + 3;
  strm.avail_in = static_cast<uInt>(len - 3);
  if (inflateInit2(&strm, 16 + MAX_WBITS) != Z_OK)
    return false;

  std::vector<unsigned char> outBuf(64 * 1024);
  out.clear();
  int ret = Z_OK;
  while (ret != Z_STREAM_END)
  {
    strm.next_out = outBuf.data();
    strm.avail_out = static_cast<uInt>(outBuf.size());
    ret = inflate(&strm, Z_NO_FLUSH);
    if (ret < 0)
      break;
    out.append(reinterpret_cast<char *>(outBuf.data()),
               outBuf.size() - static_cast<size_t>(strm.avail_out));
  }
  inflateEnd(&strm);
  return ret == Z_STREAM_END;
}
}  // namespace

namespace jni
{
std::vector<std::string> BRouterCalculateRoutes(std::vector<double> const & lats, std::vector<double> const & lons,
                                                int maxCount)
{
  ASSERT(!lats.empty() && lats.size() == lons.size(), ());
  ScopedEnv env(jni::GetJVM());
  if (!env)
  {
    LOG(LWARNING, ("BRouterServiceClient: cannot attach to JVM"));
    return {};
  }

  static jclass g_clientClass = nullptr;
  static jmethodID g_calculateRoutesId = nullptr;
  if (g_clientClass == nullptr)
    g_clientClass = GetAppClassRef(env.get(), "app.organicmaps.sdk.brouter.BRouterServiceClient");
  if (g_clientClass == nullptr)
    return {};
  if (g_calculateRoutesId == nullptr)
    g_calculateRoutesId = env->GetStaticMethodID(
        g_clientClass, "calculateRoutesBlocking",
        "(Landroid/content/Context;[D[DI)[Ljava/lang/String;");

  jobject appCtx = GetApplicationContext(env.get());
  if (appCtx == nullptr)
    return {};

  jni::ScopedLocalRef<jobject> const appCtxRef(env.get(), appCtx);

  jni::ScopedLocalRef<jdoubleArray> const jniLats(env.get(), env->NewDoubleArray(static_cast<jsize>(lats.size())));
  env->SetDoubleArrayRegion(jniLats.get(), 0, static_cast<jsize>(lats.size()), lats.data());

  jni::ScopedLocalRef<jdoubleArray> const jniLons(env.get(), env->NewDoubleArray(static_cast<jsize>(lons.size())));
  env->SetDoubleArrayRegion(jniLons.get(), 0, static_cast<jsize>(lons.size()), lons.data());

  jni::ScopedLocalRef<jobjectArray> const jResults(
      env.get(), static_cast<jobjectArray>(env->CallStaticObjectMethod(
                     g_clientClass, g_calculateRoutesId, appCtxRef.get(), jniLats.get(), jniLons.get(),
                     static_cast<jint>(maxCount))));
  if (jni::HandleJavaException(env.get()))
  {
    LOG(LWARNING, ("BRouterServiceClient.calculateRoutesBlocking threw"));
    return {};
  }
  if (!jResults.get())
    return {};

  std::vector<std::string> results;
  jsize const len = env->GetArrayLength(jResults.get());
  results.reserve(static_cast<size_t>(len));
  for (jsize i = 0; i < len; ++i)
  {
    jni::ScopedLocalRef<jstring> const jGpx(
        env.get(), static_cast<jstring>(env->GetObjectArrayElement(jResults.get(), i)));
    if (!jGpx.get())
      continue;
    std::string raw = jni::ToNativeString(env.get(), jGpx.get());
    // BRouter wraps its GPX response in an ejY0 (base64) + gzip envelope when
    // acceptCompressedResult=true. Decode before handing the GPX off to the
    // router engine.
    std::string decoded;
    if (DecodeCompressed(raw, decoded))
      raw = std::move(decoded);
    results.push_back(std::move(raw));
  }
  return results;
}
}  // namespace jni
