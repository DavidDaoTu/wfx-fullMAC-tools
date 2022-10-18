#! /bin/bash

export PATH=$PATH:$AGENT_WORKSPACE/slc_cli

echo "PROJECTS_NAME = $PROJECTS_NAME"
echo "AGENT_WORKSPACE = $AGENT_WORKSPACE"
echo "BOARD_IDs = $BOARD_IDs"
echo "GIT_BRANCH = $GIT_BRANCH"
echo "GIT_COMMIT = $GIT_COMMIT"

##### GET GIT BRANCH & COMMIT ID #####
PROJECT_BRANCH=${GIT_BRANCH//'/'/'_'}
COMMIT_ID=${GIT_COMMIT:0:8}

# GLOBAL VARIABLES #
IS_PATCHED=0


##### HELPER FUNCTIONS
# is_supported() function checks this board is supported by this project.
# Params: arg1: board_id; arg2: project_name
# Return: 0 if supported; otherwise false 

is_supported() {

    local board_id=$1
    local project_name=$2

    case "$project_name" in

    'wifi_cli_micriumos')
        # wifi_cli project does not support xG24/xG21
        if [[ "$board_id" == 'brd4187a' ]]  || [[ "$board_id" == 'brd4180a' ]]
        then
            echo "$board_id is not supported by $project_name!"      
            return 1 #false
        fi 
        ;;

    'secured_mqtt')
        # secured_mqtt project does not support xG24/xG21
        if [[ "$board_id" == 'brd4187a' ]]  || [[ "$board_id" == 'brd4180a' ]]
        then
            echo "$board_id is not supported by $project_name!"      
            return 1 #false
        fi 
        ;;

    'ethernet_bridge')
        # ethernet_bridge project only support WGM160P
        if [ "$board_id" != 'brd4321a_a06' ]
        then
            echo "$board_id is not supported by $project_name!"      
            return 1 #false
        fi        
        ;;

    'multiprotocol_micriumos')
        # multiprotocol project only support xG12/xG21/xG24
        if [[ "$board_id" == 'brd4321a_a06' ]]  || \
            [[ "$board_id" == 'brd2204a' ]] || \
            [[ "$board_id" == 'brd2204c' ]]
        then
            echo "$board_id is not supported by $project_name!"      
            return 1 #false
        fi        
        ;;    

    *)
        echo -n "unknown project: $project_name"
        return 1 #false
        ;;
    esac
    return 0 #true
}

# git_apply_reverse_patch() function apply or reverse the application patch.
#   For applying patch, called this function before project generation.
#   For reverse patch, called after building the project
# Arguments:
#   + arg1: apply_or_reverse (1: apply; 0: reverse)
#   + arg2: board_id
#   + arg3: project_name
git_apply_reverse_patch() {
    
    local apply_or_reverse=$1
    local board_id=$2
    local project_name=$3

    if [[ $project_name =~ .*"$PATCH_APPs".* ]] && \
        [[ $board_id =~ .*"$PATCH_BOARDs".* ]] && \
        [[ $apply_or_reverse -eq 1 ]]
    then
        # Apply "application" patch to the source project if brd4187a/b/c
        
        echo "Going into $project_name ...."
        cd ./$project_name
        echo "git apply --check ./patches/$PATCH_BOARDs/app.patch"
        git apply --check ./patches/$PATCH_BOARDs/app.patch
        echo "git apply ./patches/brd4187/app.patch"
        git apply ./patches/$PATCH_BOARDs/app.patch
        cd ../
        IS_PATCHED=1 #patched
    else 
        echo "Going into $project_name ...."
        cd ./$project_name
        echo "git apply -R ./patches/$PATCH_BOARDs/app.patch"
        git apply -R ./patches/$PATCH_BOARDs/app.patch
        cd ../
        IS_PATCHED=0 #not_patched
    fi
}

# override_config_files() replaces the default header files in config folder by
# our prepared files
# Arguments:
#   + arg1: board_id
#   + arg2: project_name
override_config_files() {
    local board_id=$1
    local project_name=$2

    if [[ $project_name =~ .*"$PATCH_APPs".* ]] && \
        [[ $board_id =~ .*"$PATCH_BOARDs".* ]]
    then
        echo "Copying files in $project/patches/brd4187/config to out_$project/$BRD_PRJ_NAME/config"
        ls -l $project/patches/$PATCH_BOARDs/config/*
        cp -r $project/patches/$PATCH_BOARDs/config/* out_$project/$BRD_PRJ_NAME/config
        echo "Check contents of overriden config files"
        cat out_$project/$BRD_PRJ_NAME/config/sl_iostream_eusart_vcom_config.h
        cat out_$project/$BRD_PRJ_NAME/config/sl_wfx_host_bus_pinout.h
    fi
}


##### CLONE OR PULL THE LATEST GSDK FROM GITHUB #####
if [ ! -d gecko_sdk ]
then
    echo "Cloning GSDK... from Github"
    git clone https://github.com/SiliconLabs/gecko_sdk.git
    if [ $? -ne 0 ]
    then 
        echo "Failed to clone GSDK! Exiting..."
        exit 1
    fi
fi
echo "Going to ./gecko_sdk directory & git pull"
cd ./gecko_sdk
git lfs pull origin
git log -n3
GSDK_BRANCH=`git rev-parse --abbrev-ref HEAD`
GSDK_TAG=`git describe --tag`
cd ../

##### CLEAN & CREATE THE OUTPUT FOLDER CONTAINING BINARY HEX FILE #####
rm -rf BIN_*
OUT_FOLDER=BIN_${PROJECT_BRANCH}_${COMMIT_ID}_${GSDK_BRANCH}_${GSDK_TAG}
mkdir $OUT_FOLDER

##### INITIALIZE SDK & TOOLCHAIN #####
slc signature trust --sdk ./gecko_sdk/
slc configuration --sdk ./gecko_sdk/
slc configuration --gcc-toolchain $AGENT_WORKSPACE/gnu_arm

##### SUBSTITUE SEPERATORS (,) OF PROJECT NAMES & BOARD IDs BY THE WHITE SPACES & CONVERT TO AN ARRAY #####
SEPERATOR=","
WHITESPACE=" "
projects=(${PROJECTS_NAME//$SEPERATOR/$WHITESPACE})
board_ids=(${BOARD_IDs//$SEPERATOR/$WHITESPACE})

echo "projects = $projects"
echo "board_ids = $board_ids"

##### LOOP THROUGH PROJECTS #####
for project in ${projects[@]}
do 
    echo "Procesing $project"
    if [ -d out_$project ] 
    then
        echo "Removing the older out_$project"
        rm -rf out_$project
    fi
    
    # Check if there is any patch files: driver or apps
    if [ -f ./$project/patches/driver.patch ]
    then
        echo "Copying the driver.patch file to the gecko_sdk folder!"
        cp ./$project/patches/driver.patch ./gecko_sdk
        echo "Going to gecko_sdk folder & applying driver.patch file to wfx-fmac-driver component!"
        cd ./gecko_sdk
        echo "Running: git apply --stat driver.patch"
        git apply --stat driver.patch
        echo "Running: git apply --check driver.patch"
        git apply --check driver.patch        
        res=$?
        if [ $res -ne 0 ]
        then
            echo "#NOTICE: There are 02 possibilities of errors: Failed to apply or already applied driver patch file of the $project project!!!"
            echo "#WARNING: If the patch have been already applied, the project can be built successfully! Don't worry!"
            echo "#ERROR: If failed to apply the patch, the project build would be failed! Must worry about applying the patch file successfully!"
            cd ../
            continue # skips, don't apply the patch file
        fi

        echo "Running: git apply driver.patch"
        git apply driver.patch
        
        cd ../
        echo "Going back wfx-fullMAC-tools repo"
    fi
    
    # Create the output project folder containing all board-support generated projects
    mkdir out_$project && mkdir ./$OUT_FOLDER/out_$project

    ###### Loop through boards ######
    for board_id in ${board_ids[@]}
    do
        echo "Processing board_id = $board_id of the $project ..."

        # Skips boards which are not supported by the current processing project
        is_supported $board_id $project
        res=$?
        if [ $res -ne 0 ]
        then
            echo "$board_id is not supported by $project project!"      
            continue
        fi

        # Check & apply git patch file. Calling this before project generation
        git_apply_reverse_patch 1 $board_id $project

        # Creating a output folder containing generated project
        BRD_PRJ_NAME=${board_id}_${project}
        mkdir ./out_$project/$BRD_PRJ_NAME

        # Generating the projects by slcp tool
        echo "Generating a new out_$project/$BRD_PRJ_NAME"
        slc generate --generator-timeout=180 ./$project/$project.slcp \
                    -np -d out_$project/$BRD_PRJ_NAME \
                    -o makefile --with $board_id
        if [ $? -ne 0 ];then
            echo "Failed to generate $BRD_PRJ_NAME! Exiting.."
            exit 1
        fi

        # Copy the config_files to override the default config files. Called this
        # function after project generation & the project is patched
        if [ $IS_PATCHED -eq 1 ]
        then   
            override_config_files $board_id $project_name
        fi

        # Going to generated project & Build the project
        echo "Going to the out_$project/$BRD_PRJ_NAME & building"
        cd ./out_$project/$BRD_PRJ_NAME
        echo "===================> Begin <===================="
        make -j12 -f $project.Makefile clean all
        
        if [ $? -eq 0 ];then
            # Copy the built binary file to output folder & add md5sum 
            cp build/debug/*.hex ../../$OUT_FOLDER/out_$project/$BRD_PRJ_NAME.hex            
            md5sum ../../$OUT_FOLDER/out_$project/$BRD_PRJ_NAME.hex >> ../../$OUT_FOLDER/md5sum_check
        else
            echo "Failed to build $BRD_PRJ_NAME! Exiting..."
            exit 1
        fi    
        echo "===================> Finished <=================="
        cd ../../

        # Reverse git apply patch.
        if [ $IS_PATCHED -eq 1 ]
        then   
            git_apply_reverse_patch 0 $board_id $project
        fi
    done
done

##### PACKAGING THE BINARY OUTPUT FILES #####
tar -cvf $OUT_FOLDER.tar.gz $OUT_FOLDER/*
