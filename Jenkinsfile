// Variables
def SLAVE_LABEL = "tudao-pc-ubuntu"

pipeline {
    environment {
        AGENT_WORKSPACE = '/home/root'
        PROJECTS_NAME = 'secured_mqtt:wifi_cli_icriumos:ethernet_bridge'
        BOARD_ID = 'brd4321a_a06'
    }

    agent {
        node {
            label "${SLAVE_LABEL}"
        }
    }

    stages {
        stage('Build') {

            agent {
                dockerfile {
                    args '-u 0:0'
                    reuseNode true
                }
            }

            steps {
                sh 'chmod a+x build_script.sh'
                sh './build_script.sh'
            }
        }
    }

    post {
        always {
            archiveArtifacts artifacts: '**/BIN_*.zip',
                                allowEmptyArchive: false,
                                fingerprint: true,
                                onlyIfSuccessful: true
        }
    }
}
