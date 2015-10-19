#ifndef NF_TYPE_H_
#define NF_TYPE_H_ 1

#include <string>
#include <assert.h>

using namespace std;

typedef enum{
	DPDK,
	DOCKER,
#ifdef ENABLE_KVM
	KVM
#endif
	//[+] Add here other implementations for the execution environment
	}nf_t;

class NFType
{
public:
	static string toString(nf_t type)
	{
		if(type == DPDK)
			return string("dpdk");
#ifdef ENABLE_DOCKER
		else if(type == DOCKER)
			return string("docker");
#endif
#ifdef ENABLE_KVM
		else if(type == KVM)
			return string("kvm");
#endif		

		assert(0);
		return "";
	}
	
	static unsigned int toID(nf_t type)
	{
		if(type == DPDK)
			return 2;
#ifdef ENABLE_DOCKER
		else if(type == DOCKER)
			return 1;
#endif
#ifdef ENABLE_KVM
		else if(type == KVM)
			return 0;
#endif		

		assert(0);
		return 0;
	}

	static bool isValid(string type)
	{
		if(type == "dpdk" 
#ifdef ENABLE_DOCKER		
		|| type == "docker"
#endif		
#ifdef ENABLE_KVM
		|| type == "kvm"
#endif				
		)
			return true;
	
		assert(0);
		
		return false;
	}
};

#endif //NF_TYPE_H_
