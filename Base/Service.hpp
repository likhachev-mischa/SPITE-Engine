#pragma once
namespace spite
{
	struct Service
	{
		virtual ~Service() = default;

		virtual void init(void* configuration)
		{
		}

		virtual void shutdown()
		{
		}
	};

#define SPITE_DECLARE_SERVICE(Type) static Type* instance();
}
