#pragma once
#include "toolchain.h"
namespace iris {
	class android_tc : public tool_chain {
	public:
		android_tc();
					~android_tc() override;
		void        inject_vm(vm*) const;
		void        build(scope* proj_scope) override;
		language    get_language() const override;
		platform    get_platform() const override;

	protected:
		void		initialize();

	private:
		string		m_android_home;
		string		m_java_home;
		string_list	m_platforms;
		string_list	m_build_tools;
	};
}