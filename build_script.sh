#! /bin/bash

export PATH=$PATH:$AGENT_WORKSPACE/slc_cli

echo "BUILD_VERSION = $BUILD_VERSION"
echo "PROJECTS_NAME = $PROJECTS_NAME"
echo "PATH = $PATH"
echo "AGENT_WORKSPACE = $AGENT_WORKSPACE"
echo "BOARD_ID = $BOARD_ID"
echo "make --version"
make --version
echo "git --version"
git --version

##### Get git branch & commit ID #####
BRANCH=`git rev-parse --abbrev-ref HEAD`
PROJECT_BRANCH=${BRANCH//'/'/'_'}
COMMIT_ID=`git rev-parse HEAD | cut -c -7`

##### Clone/pull the latest GSDK from github #####
if [ ! -d gecko_sdk ]
then
    echo "Cloning GSDK..."
    git lfs clone https://github.com/SiliconLabs/gecko_sdk.git
    if [ $? -ne 0]
    then 
        echo "Failed to clone GSDK! Exiting..."
        exit 1
    fi
fi
echo "Going to ./gecko_sdk & git pull"
cd ./gecko_sdk
git lfs pull origin
git log -n3
GSDK_BRANCH=`git rev-parse --abbrev-ref HEAD`
GSDK_TAG=`git describe --tag`
cd ../

##### CLEAN & CREATE OUTPUT FOLDER CONTAINING BINARY HEX FILE #####
rm -rf BIN_*
OUT_FOLDER=BIN_${GSDK_BRANCH}_${GSDK_TAG}_${PROJECT_BRANCH}_${COMMIT_ID}
mkdir $OUT_FOLDER

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
    slc generate ./$project/$project.slcp -np -d out_$project/ -o makefile --with $BOARD_ID

    # Building the projects
    echo "Going out_$project & building"
    cd ./out_$project
    echo "===================> Begin <===================="
    make -j12 -f $project.Makefile clean all
    if [ $? -eq 0 ];then
        cp build/debug/*.hex ../$OUT_FOLDER
    fi    
    echo "===================> Finished <=================="
    cd ../    
done

tar cvf $OUT_FOLDER.zip $OUT_FOLDER/*

# commander flash build/debug/ethernet_bridge.hex