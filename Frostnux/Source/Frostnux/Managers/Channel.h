#pragma once

namespace Frostnux
{

	enum class Channel
	{
		Stable = 0,
		Preview
	};

	class ChannelManager
	{
	public:
		static Channel GetChannel() { return s_Channel; }
		static void SetChannel(Channel channel) { s_Channel = channel; }
	private:
		ChannelManager() = default;

		ChannelManager(const ChannelManager&) = delete;
		ChannelManager& operator=(const ChannelManager&) = delete;
		ChannelManager(ChannelManager&&) = delete;
		ChannelManager& operator=(ChannelManager&&) = delete;
		ChannelManager& operator=(const ChannelManager&&) = delete;
		static Channel s_Channel;
	};

}
