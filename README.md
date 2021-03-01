# segy-software
Vac undetectable external csgo cheat

This is csgo external ESP (enemy players, hostages, bomb) with triggerbot and recoil crosshair. It has 3 parts which make it totaly undetectable for vac (unless they get hash of compiled binaries on your disk):

- SegyLoader - executable that injects folowing 2 dlls (do it before you start steam) and than exits so after starting steam and csgo no cheat process is running on machine
- SegyExplorer - this is overlay dll that is injected into explorer and creates transparent fullscreen window for wallhack render (yes signed explorer by micro$oft can do it, bypasses vac, no screenshots are ever taken)
- SegyDll - this is the cheat itself, dynamic link library that is injected in service host and hijacks open handle to csgo process for VM read and handle to explorer for communication

After reaching rank Global Elite i got bored and quit csgo so i decided to release this private hack to public. Have fun!
