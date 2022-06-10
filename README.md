# Flatpak Permissions Management KCM

Still under development.

## Building and Running:

- cd wherever/you/like
- git clone [ the link you copy from the clone option ]
- cd flatpak_kcm
- mkdir build
- cd build
- cmake .. -DCMAKE_INSTALL_PREFIX=~/.local/kde
- make -j8 install
- source prefix.sh
- kcmshell5 kcm_flatpak
