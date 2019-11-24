#pragma once

#if BUILD_SHARED
#ifndef IBVM_API
#ifdef _MSC_VER
#define IBVM_API __declspec(dllexport)
#else
#define IBVM_API __attribute__((visibility("default")))
#endif
#endif
#else
#ifndef IBVM_API
#ifdef _MSC_VER
#define IBVM_API __declspec(dllimport)
#else
#define IBVM_API
#endif
#endif
#endif
