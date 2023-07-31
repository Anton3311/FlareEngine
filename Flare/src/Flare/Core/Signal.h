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

		void Invoke(const T&& ...args)
		{
			for (auto& listener : m_Listeners)
				listener(std::forward<T>(args)...);
		}

		void Bind(const ListenerFunction& listener)
		{
			m_Listeners.push_back(listener);
		}
	private:
		std::vector<ListenerFunction> m_Listeners;
	};
}