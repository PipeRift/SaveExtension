# Asynchronous

Asynchronous loading and saving is one of the supported features of the plugin.

Saving process can be streamed between multiple frames so that no frame is lost.

{% hint style='warning' %} For now, async is not recommended while level streaming saving is enabled. It could be interrupted while loading or saving creating unexpected issues {% endhint %}