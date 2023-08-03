#include "FlareScriptingCore/Flare.h"

namespace Sandbox
{
	struct ExecutionOrder_TestComponent
	{
		FLARE_COMPONENT(ExecutionOrder_TestComponent);
		FLARE_DEFAULT_SERIALIZER;
	};
	FLARE_COMPONENT_IMPL(ExecutionOrder_TestComponent, ExecutionOrder_TestComponent::ConfigureSerialization);

	struct ExecutionOrder_TestComponent1
	{
		FLARE_COMPONENT(ExecutionOrder_TestComponent1);
		FLARE_DEFAULT_SERIALIZER;
	};
	FLARE_COMPONENT_IMPL(ExecutionOrder_TestComponent1, ExecutionOrder_TestComponent::ConfigureSerialization);

	struct ExecutionOrder_TestComponent2
	{
		FLARE_COMPONENT(ExecutionOrder_TestComponent2);
		FLARE_DEFAULT_SERIALIZER;
	};
	FLARE_COMPONENT_IMPL(ExecutionOrder_TestComponent2, ExecutionOrder_TestComponent::ConfigureSerialization);

	struct FirstSystem : public Flare::SystemBase
	{
		FLARE_SYSTEM(FirstSystem);
		FLARE_DEFAULT_SERIALIZER;

		virtual void Configure(Flare::SystemConfiguration& config) override
		{
			config.Query.With<ExecutionOrder_TestComponent>();
		}

		virtual void Execute(Flare::EntityView& chunk) override
		{
			FLARE_INFO("First");
		}
	};
	FLARE_SYSTEM_IMPL(FirstSystem);

	struct SecondSystem : public Flare::SystemBase
	{
		FLARE_SYSTEM(SecondSystem);
		FLARE_DEFAULT_SERIALIZER;

		virtual void Configure(Flare::SystemConfiguration& config) override
		{
			config.Query.With<ExecutionOrder_TestComponent1>();

		}

		virtual void Execute(Flare::EntityView& chunk) override
		{
			FLARE_INFO("Second");
		}
	};
	FLARE_SYSTEM_IMPL(SecondSystem);

	struct ThirdSystem : public Flare::SystemBase
	{
		FLARE_SYSTEM(ThirdSystem);
		FLARE_DEFAULT_SERIALIZER;

		virtual void Configure(Flare::SystemConfiguration& config) override
		{
			config.Query.With<ExecutionOrder_TestComponent2>();

			config.ExecuteBefore<FirstSystem>();
			config.ExecuteAfter<SecondSystem>();
		}

		virtual void Execute(Flare::EntityView& chunk) override
		{
			FLARE_INFO("Third");
		}
	};
	FLARE_SYSTEM_IMPL(ThirdSystem);
}