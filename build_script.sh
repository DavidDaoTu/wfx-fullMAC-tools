#! /bin/sh

export PATH=$PATH:$AGENT_WORKSPACE/slc_cli

echo "PROJECTS_NAME = $PROJECTS_NAME"
echo "PATH = $PATH"
echo "In build script AGENT_WORKSPACE = $AGENT_WORKSPACE"

# Clone/Pull GSDK from github
if [ -d gecko_sdk ]
then
    echo "Going to ./gecko_sdk & git pull"
    cd ./gecko_sdk
    git pull
    git log    
    cd ../
else
    echo "git clone by https"
    git clone https://github.com/SiliconLabs/gecko_sdk.git
    git log
fi

echo "Running ls -la"
ls -la

echo "PWD = $PWD"

# Initialize SLC tool
slc signature trust --sdk ./gecko_sdk/
slc configuration --sdk ./gecko_sdk/
slc configuration --gcc-toolchain $AGENT_WORKSPACE/gnu_arm

# Clear the output folder
if [ -d out_ethernet_bridge ] 
then
    echo "Removing out_ethernet_bridge"
    rm -rf out_ethernet_bridge
fi
mkdir out_ethernet_bridge

slc generate ./ethernet_bridge/ethernet_bridge.slcp -np -d out_ethernet_bridge/ -o makefile --with brd4321a_a06

cd ./out_ethernet_bridge
make -j12 -f ethernet_bridge.Makefile clean all

# commander flash build/debug/ethernet_bridge.hex