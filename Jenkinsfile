pipeline {
    
    environment {
        AGENT_WORKSPACE = '/home/root'
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
            //archiveArtifacts artifacts: 'build/libs/**/*.jar', fingerprint: true
            archiveArtifacts '**/build/debug/*.hex'
        }
    }
}
