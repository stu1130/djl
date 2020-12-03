/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You may not use this file except in compliance
 * with the License. A copy of the License is located at
 *
 * http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES
 * OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions
 * and limitations under the License.
 */
#include "ai_djl_dlr_jni_DlrLibrary.h"
#include "iostream"
#include <jni.h>
#include <dlfcn.h>
#include <vector>
#include <unordered_map>

typedef void* DLRModelHandle;
typedef void* DlrRuntimeHandle;
typedef int (*GetNumInputs)(DLRModelHandle*, int*);
typedef int (*GetNumWeights)(DLRModelHandle*, int*);
typedef int (*GetInputName)(DLRModelHandle*, int, const char**);
typedef int (*GetWeightName)(DLRModelHandle*, int, const char**);
typedef int (*SetInput)(DLRModelHandle*, const char*, const int64_t*, const void*, int);
typedef int (*GetInput)(DLRModelHandle*, const char*, void*);
typedef int (*GetOutputShape)(DLRModelHandle*, int, int64_t*);
typedef int (*GetOutput)(DLRModelHandle*, int, void*);
typedef int (*GetOutputSizeDim)(DLRModelHandle*, int, int64_t*, int*);
typedef int (*GetNumOutputs)(DLRModelHandle*, int*);
typedef int (*CreateModel)(DLRModelHandle*, const char*, int, int);
typedef int (*DeleteModel)(DLRModelHandle*);
typedef int (*RunModel)(DLRModelHandle*);
typedef const char* (*GetLastError)();
typedef int (*GetBackend)(DLRModelHandle*, const char**);
typedef int (*SetNumThreads)(DLRModelHandle*m, int);
typedef int (*UseCPUAffinity)(DLRModelHandle*, int);

struct DlrApis {
  void* dlr_runtime_handle;
  int (*GetNumInputs)(DLRModelHandle*, int*);
  int (*GetNumWeights)(DLRModelHandle*, int*);
  int (*GetInputName)(DLRModelHandle*, int, const char**);
  int (*GetWeightName)(DLRModelHandle*, int, const char**);
  int (*SetInput)(DLRModelHandle*, const char*, const int64_t*, const void*, int);
  int (*GetInput)(DLRModelHandle*, const char*, void*);
  int (*GetOutputShape)(DLRModelHandle*, int, int64_t*);
  int (*GetOutput)(DLRModelHandle*, int, void*);
  int (*GetOutputSizeDim)(DLRModelHandle*, int, int64_t*, int*);
  int (*GetNumOutputs)(DLRModelHandle*, int*);
  int (*CreateModel)(DLRModelHandle*, const char*, int, int);
  int (*DeleteModel)(DLRModelHandle*);
  int (*RunModel)(DLRModelHandle*);
  const char* (*GetLastError)();
  int (*GetBackend)(DLRModelHandle*, const char**);
  int (*SetNumThreads)(DLRModelHandle*m, int);
  int (*UseCPUAffinity)(DLRModelHandle*, int);
};

// global variable for dlr_models
static std::unordered_map<uintptr_t, DlrApis> dlr_models;

inline DlrApis LoadDlrApis(JNIEnv* env, void* dlr_runtime_handle) {
  DlrApis api_table{
    dlr_runtime_handle,
    (GetNumInputs) dlsym(dlr_runtime_handle, "GetDLRNumInputs"),
    (GetNumWeights) dlsym(dlr_runtime_handle, "GetDLRNumWeights"),
  (GetInputName) dlsym(dlr_runtime_handle, "GetDLRInputName"),
  (GetWeightName) dlsym(dlr_runtime_handle, "GetDLRWeightName"),
  (SetInput) dlsym(dlr_runtime_handle, "SetDLRInput"),
  (GetInput) dlsym(dlr_runtime_handle, "GetDLRInput"),
  (GetOutputShape) dlsym(dlr_runtime_handle, "GetDLROutputShape"),
  (GetOutput) dlsym(dlr_runtime_handle, "GetDLROutput"),
  (GetOutputSizeDim) dlsym(dlr_runtime_handle, "GetDLROutputSizeDim"),
  (GetNumOutputs) dlsym(dlr_runtime_handle, "GetDLRNumOutputs"),
  (CreateModel) dlsym(dlr_runtime_handle, "CreateDLRModel"),
  (DeleteModel) dlsym(dlr_runtime_handle, "DeleteDLRModel"),
  (RunModel) dlsym(dlr_runtime_handle, "RunDLRModel"),
  (GetLastError) dlsym(dlr_runtime_handle, "GetDLRLastError"),
  (GetBackend) dlsym(dlr_runtime_handle, "GetDLRBackend"),
  (SetNumThreads) dlsym(dlr_runtime_handle, "SetDLRNumThreads"),
  (UseCPUAffinity) dlsym(dlr_runtime_handle, "UseDLRCPUAffinity")};
  const char *dlsym_error = dlerror();
  if (dlsym_error) {
    dlclose(dlr_runtime_handle);
    jclass jexception = env->FindClass("ai/djl/engine/EngineException");
    env->ThrowNew(jexception, "can't load dlr library symbol");
  }
  return api_table;
}

inline DlrRuntimeHandle LoadDlr(JNIEnv *env, jlong id) {
  std::string path = "/Users/leecheng/.djl.ai/dlr/1.5.0-SNAPSHOT-cpu-osx-x86_64/libdlr_" + std::to_string(id) + ".dylib";
  DlrRuntimeHandle dlr_runtime_handle = dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
  if (!dlr_runtime_handle) {
    jclass jexception = env->FindClass("ai/djl/engine/EngineException");
    env->ThrowNew(jexception, "can't load dlr library");
  }
  return dlr_runtime_handle;
}

inline void CheckStatus(JNIEnv *env, GetLastError func, int status) {
  if (status) {
    jclass jexception = env->FindClass("ai/djl/engine/EngineException");
    const char* err = func();
    env->ThrowNew(jexception, err);
  }
}

JNIEXPORT jint JNICALL Java_ai_djl_dlr_jni_DlrLibrary_getDlrNumInputs(JNIEnv* env, jobject jthis, jlong jhandle) {
  int num;
  auto* handle = reinterpret_cast<DLRModelHandle*>(jhandle);
  DlrApis api_table = dlr_models[jhandle];
  CheckStatus(env, api_table.GetLastError, api_table.GetNumInputs(handle, &num));
  return num;
}

JNIEXPORT jint JNICALL Java_ai_djl_dlr_jni_DlrLibrary_getDlrNumWeights(JNIEnv* env, jobject jthis, jlong jhandle) {
  int num;
  auto* handle = reinterpret_cast<DLRModelHandle*>(jhandle);
  DlrApis api_table = dlr_models[jhandle];
  CheckStatus(env, api_table.GetLastError, api_table.GetNumWeights(handle, &num));
  return num;
}

JNIEXPORT jstring JNICALL Java_ai_djl_dlr_jni_DlrLibrary_getDlrInputName(
    JNIEnv* env, jobject jthis, jlong jhandle, jint jindex) {
  const char* name;
  auto* handle = reinterpret_cast<DLRModelHandle*>(jhandle);
  DlrApis api_table = dlr_models[jhandle];
  CheckStatus(env, api_table.GetLastError, api_table.GetInputName(handle, jindex, &name));
  return env->NewStringUTF(name);
}

JNIEXPORT jstring JNICALL Java_ai_djl_dlr_jni_DlrLibrary_getDlrWeightName(
    JNIEnv* env, jobject jthis, jlong jhandle, jint jindex) {
  const char* name;
  auto* handle = reinterpret_cast<DLRModelHandle*>(jhandle);
  DlrApis api_table = dlr_models[jhandle];
  CheckStatus(env, api_table.GetLastError, api_table.GetWeightName(handle, jindex, &name));
  return env->NewStringUTF(name);
}

JNIEXPORT void JNICALL Java_ai_djl_dlr_jni_DlrLibrary_setDLRInput(
    JNIEnv* env, jobject jthis, jlong jhandle, jstring jname, jlongArray jshape, jfloatArray jinput, jint jdim) {
  jfloat* input_body = env->GetFloatArrayElements(jinput, JNI_FALSE);
  jlong* shape_body = env->GetLongArrayElements(jshape, JNI_FALSE);
  const char* name = env->GetStringUTFChars(jname, JNI_FALSE);
  auto* handle = reinterpret_cast<DLRModelHandle*>(jhandle);
  DlrApis api_table = dlr_models[jhandle];
  CheckStatus(env, api_table.GetLastError, api_table.SetInput(handle, name, reinterpret_cast<int64_t *>(shape_body), input_body, jdim));
  env->ReleaseFloatArrayElements(jinput, input_body, JNI_ABORT);
  env->ReleaseLongArrayElements(jshape, shape_body, JNI_ABORT);
  env->ReleaseStringUTFChars(jname, name);
}

JNIEXPORT jlongArray JNICALL Java_ai_djl_dlr_jni_DlrLibrary_getDlrOutputShape(
    JNIEnv* env, jobject jthis, jlong jhandle, jint jindex) {
  auto* handle = reinterpret_cast<DLRModelHandle*>(jhandle);
  DlrApis api_table = dlr_models[jhandle];
  int64_t size;
  int dim;
  CheckStatus(env, api_table.GetLastError, api_table.GetOutputSizeDim(handle, jindex, &size, &dim));
  jlong shape[dim];
  CheckStatus(env, api_table.GetLastError, api_table.GetOutputShape(handle, jindex, reinterpret_cast<int64_t *>(shape)));
  jlongArray res = env->NewLongArray(dim);
  env->SetLongArrayRegion(res, 0, dim, shape);
  return res;
}

JNIEXPORT jfloatArray JNICALL Java_ai_djl_dlr_jni_DlrLibrary_getDlrOutput(
    JNIEnv* env, jobject jthis, jlong jhandle, jint jindex) {
  auto* handle = reinterpret_cast<DLRModelHandle*>(jhandle);
  DlrApis api_table = dlr_models[jhandle];
  int64_t size;
  int dim;
  CheckStatus(env, api_table.GetLastError, api_table.GetOutputSizeDim(handle, jindex, &size, &dim));
  float data[size];
  CheckStatus(env, api_table.GetLastError, api_table.GetOutput(handle, jindex, data));
  jfloatArray res = env->NewFloatArray(size);
  env->SetFloatArrayRegion(res, 0, size, data);
  return res;
}

JNIEXPORT jint JNICALL Java_ai_djl_dlr_jni_DlrLibrary_getDlrNumOutputs(JNIEnv* env, jobject jthis, jlong jhandle) {
  int num;
  auto* handle = reinterpret_cast<DLRModelHandle*>(jhandle);
  DlrApis api_table = dlr_models[jhandle];
  CheckStatus(env, api_table.GetLastError, api_table.GetNumOutputs(handle, &num));
  return num;
}

JNIEXPORT jlong JNICALL Java_ai_djl_dlr_jni_DlrLibrary_createDlrModel(
    JNIEnv* env, jobject jthis, jlong jid, jstring jmodel_path, jint jdev_type, jint jdev_id) {
  const char* model_path = env->GetStringUTFChars(jmodel_path, JNI_FALSE);
  auto* handle = new DLRModelHandle();
  DlrRuntimeHandle dlr_runtime_handle = LoadDlr(env, jid);
  DlrApis api_table = LoadDlrApis(env, dlr_runtime_handle);
  CheckStatus(env, api_table.GetLastError, api_table.CreateModel(handle, model_path, jdev_type, jdev_id));
  auto jhandle = reinterpret_cast<uintptr_t>(handle);
  dlr_models[jhandle] = api_table;
  return jhandle;
}

JNIEXPORT void JNICALL Java_ai_djl_dlr_jni_DlrLibrary_deleteDlrModel(JNIEnv* env, jobject jthis, jlong jhandle) {
  auto* handle = reinterpret_cast<DLRModelHandle*>(jhandle);
  DlrApis api_table = dlr_models[jhandle];
  CheckStatus(env, api_table.GetLastError, api_table.DeleteModel(handle));
  dlclose(api_table.dlr_runtime_handle);
}

JNIEXPORT void JNICALL Java_ai_djl_dlr_jni_DlrLibrary_runDlrModel(JNIEnv* env, jobject jthis, jlong jhandle) {
  auto* handle = reinterpret_cast<DLRModelHandle*>(jhandle);
  DlrApis api_table = dlr_models[jhandle];
  CheckStatus(env, api_table.GetLastError, api_table.RunModel(handle));
}

JNIEXPORT jstring JNICALL Java_ai_djl_dlr_jni_DlrLibrary_getDlrBackend(JNIEnv* env, jobject jthis, jlong jhandle) {
  const char* name;
  auto* handle = reinterpret_cast<DLRModelHandle*>(jhandle);
  DlrApis api_table = dlr_models[jhandle];
  CheckStatus(env, api_table.GetLastError, api_table.GetBackend(handle, &name));
  return env->NewStringUTF(name);
}

JNIEXPORT void JNICALL Java_ai_djl_dlr_jni_DlrLibrary_setDlrNumThreads(
    JNIEnv* env, jobject jthis, jlong jhandle, jint jthreads) {
  auto* handle = reinterpret_cast<DLRModelHandle*>(jhandle);
  DlrApis api_table = dlr_models[jhandle];
  CheckStatus(env, api_table.GetLastError, api_table.SetNumThreads(handle, jthreads));
}

JNIEXPORT void JNICALL Java_ai_djl_dlr_jni_DlrLibrary_useDlrCPUAffinity(
    JNIEnv* env, jobject jthis, jlong jhandle, jboolean juse) {
  auto* handle = reinterpret_cast<DLRModelHandle*>(jhandle);
  DlrApis api_table = dlr_models[jhandle];
  CheckStatus(env, api_table.GetLastError, api_table.UseCPUAffinity(handle, juse));
}
