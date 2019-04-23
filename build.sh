# sudo apt-get install build-essential gcc-multilib g++-multilib

npm install --ignore-scripts

node-gyp rebuild --target=$1 --arch=x64 --dist-url=https://atom.io/download/electron
