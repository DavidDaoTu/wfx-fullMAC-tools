pipeline {
    environment {
        AGENT_WORKSPACE = '/home/root'
        PROJECTS_NAME = 'secured_mqtt:wifi_cli_micriumos:ethernet_bridge'
        SLAVE_LABEL = 'tudao-pc-ubuntu'
    }

    agent {
        node {
            label 'tudao-pc-ubuntu'
        }
    }

    stages {
        stage('Build') {

            agent {
                dockerfile {
                    args '-u 0:0'
                    additionalBuildArgs '--build-arg BUILD_VERSION=1.0.2'
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
