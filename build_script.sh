#! /bin/bash

export PATH=$PATH:$AGENT_WORKSPACE/slc_cli

echo "BUILD_VERSION = $BUILD_VERSION"
echo "PROJECTS_NAME = $PROJECTS_NAME"
echo "PATH = $PATH"
echo "AGENT_WORKSPACE = $AGENT_WORKSPACE"
echo "make --version"
make --version

##### Clone/pull the latest GSDK from github #####
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

##### For testing #####
echo "Running ls -la"
ls -la

echo "PWD = $PWD"

##### Initialize SDK & toolchain #####
slc signature trust --sdk ./gecko_sdk/
slc configuration --sdk ./gecko_sdk/
slc configuration --gcc-toolchain $AGENT_WORKSPACE/gnu_arm

##### Clear & create the output folders #####
# if [ -d out_ethernet_bridge ] 
# then
#     echo "Removing out_ethernet_bridge"
#     rm -rf out_ethernet_bridge
# fi
# mkdir out_ethernet_bridge
# # Generating the projects
# slc generate ./ethernet_bridge/ethernet_bridge.slcp -np -d out_ethernet_bridge/ -o makefile --with brd4321a_a06

# # Building the projects
# cd ./out_ethernet_bridge
# make -j12 -f ethernet_bridge.Makefile clean all

##### Substitue seperators by the white spaces & convert to an array #####
SEPERATOR=":"
WHITESPACE=" "
projects=(${PROJECTS_NAME//$SEPERATOR/$WHITESPACE})

##### Loop through projects #####
for project in ${projects[@]}
do 
    echo $project
    if [ -d out_$project ] 
    then
        echo "Removing the previous out_$project"
        rm -rf out_$project
    fi

    # Create output project folder
    mkdir out_$project

    # Generating the projects
    echo "Generating a new out_$project"
    slc generate ./$project/$project.slcp -np -d out_$project/ -o makefile --with brd4321a_a06

    # Building the projects
    echo "Going out_$project & building"
    cd ./out_$project
    echo "===================> Begin <===================="
    make -j12 -f $project.Makefile clean all
    echo "===================> Finished <=================="
    cd ../
    echo "Going back & ls -la.."
    echo "PWD = $PWD"
    ls -la
done

# commander flash build/debug/ethernet_bridge.hex