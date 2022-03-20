pipeline {
    
    environment {
        AGENT_WORKSPACE = '/home/root'
        PROJECTS_NAME = 'ethernet_bridge:secured_mqtt:wifi_cli_micriumos'
    }

    // agent {
    //     dockerfile true
    // }

    agent any

    stages {
        stage('Build') {

            agent {
                docker {
                    image 'davidfullstack/build_env_agent:ver0.3-openjdk11-gnu10.3'
                    args '-u 0:0'
                    reuseNode true
                }
            }

            steps {
                sh './build_script.sh'
            }
        }
    }

    post {
        always {
            archiveArtifacts artifacts: '**/build/debug/*.hex',
                                allowEmptyArchive: false,
                                fingerprint: true,
                                onlyIfSuccessful: true
        }
    }
}
