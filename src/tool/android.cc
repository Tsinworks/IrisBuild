#include "android.h"
#include "os.hpp"
#include "log.hpp"
namespace iris
{
	android_tc::android_tc()
	{
		initialize();
	}
	android_tc::~android_tc()
	{
	}
	void android_tc::inject_vm(vm *) const
	{
	}
	void android_tc::build(scope * proj_scope)
	{
	}
	language android_tc::get_language() const
	{
		return language::java;
	}
	platform android_tc::get_platform() const
	{
		return platform::android;
	}
	void android_tc::initialize()
	{
		m_java_home = getenv("JAVA_HOME");
		m_android_home = getenv("ANDROID_HOME");
		if (m_android_home.empty()) {
			m_android_home = getenv("ANDROID_SDK_ROOT");
		}
		if (m_android_home.empty()) {
			XB_LOGW("warning: ANDROID_HOME or ANDROID_SDK_ROOT is not existed in environment variables, android java project won't be built !");
			return;
		}
		if (m_java_home.empty()) {
			XB_LOGW("warning: JAVA_HOME is not existed in $PATH environment variables, android java project won't be built !");
			return;
		}
		string platform = path::join(m_android_home, "platforms");
		string build_tools = path::join(m_android_home, "build_tools");
		path::list_dirs(platform);
		path::list_dirs(build_tools);
	}
}
