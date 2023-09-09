#pragma once

#include <functional>
#include <vector>

namespace Flare
{
	template<typename... T>
	class Signal
	{
	public:
		using ListenerFunction = std::function<void(T...)>;

		template<typename... ArgT>
		void Invoke(ArgT&&... args)
		{
			for (auto& listener : m_Listeners)
				listener(std::forward<ArgT>(args)...);
		}

		void Bind(const ListenerFunction& listener)
		{
			m_Listeners.push_back(listener);
		}
	private:
		std::vector<ListenerFunction> m_Listeners;
	};
}