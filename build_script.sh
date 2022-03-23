#! /bin/bash

export PATH=$PATH:$AGENT_WORKSPACE/slc_cli

echo "BUILD_VERSION = $BUILD_VERSION"
echo "PROJECTS_NAME = $PROJECTS_NAME"
echo "PATH = $PATH"
echo "AGENT_WORKSPACE = $AGENT_WORKSPACE"
echo "make --version"
make --version
echo "git --version"
git --version

##### Clone/pull the latest GSDK from github #####
if [ -d gecko_sdk ]
then
    echo "Going to ./gecko_sdk & git pull"
    cd ./gecko_sdk
    git lfs pull origin
    git log    
    cd ../
else
    echo "git clone by https"
    git lfs clone https://github.com/SiliconLabs/gecko_sdk.git
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
done

# commander flash build/debug/ethernet_bridge.hex