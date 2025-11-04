folder('Whanos_Base_Image') {
    displayName('Whanos_Base_Image')
    description('Folder to contain all the base images of the whanos project')
}

folder('Projects') {
    displayName('Projects')
    description('Folder to contain whanos compatible projects')
}

job('Whanos_Base_Image/whanos-c') {
    steps {
        shell('docker build -t whanos-c - < ../../../images/c/Dockerfile.base')
    }
}

job('Whanos_Base_Image/whanos-befunge') {
    steps {
        shell('docker build -t whanos-befunge - < ../../../images/befunge/Dockerfile.base')
    }
}

job('Whanos_Base_Image/whanos-java') {
    steps {
        shell('docker build -t whanos-java - < ../../../images/java/Dockerfile.base')
    }
}

job('Whanos_Base_Image/whanos-javascript') {
    steps {
        shell('docker build -t whanos-javascript - < ../../../images/javascript/Dockerfile.base')
    }
}

job('Whanos_Base_Image/whanos-python') {
    steps {
        shell('docker build -t whanos-python - < ../../../images/python/Dockerfile.base')
    }
}

job('Whanos_Base_Image/Build-All-Jobs') {
    publishers {
        downstream('Whanos_Base_Image/whanos-c')
        downstream('Whanos_Base_Image/whanos-befunge')
        downstream('Whanos_Base_Image/whanos-java')
        downstream('Whanos_Base_Image/whanos-javascript')
        downstream('Whanos_Base_Image/whanos-python')
    }
}

job('/link-project') {
    parameters{
        stringParam('GIT_URL', null, 'Git URL starting from domain name upto repository name')
        stringParam('DISPLAY_NAME', null, 'Display Name')
        stringParam('GIT_USERNAME', null, 'Git user username')
        stringParam('GIT_PERSONAL_ACCESS_TOKEN', null, 'Git user personnal access token')
    }
    steps {
        dsl('''
job("/Projects/$DISPLAY_NAME") {
    scm {
        git {
            remote {
                name('origin')
                url("https://$GIT_USERNAME:$GIT_PERSONAL_ACCESS_TOKEN@$GIT_URL")
            }
        }
    }
    triggers {
        scm('* * * * *')
    }
    wrappers {
        preBuildCleanup()
    }
    steps {
        shell('cp $JENKINS_HOME/config/checkLanguage.sh checkLanguage.sh && chmod +x checkLanguage.sh && ./checkLanguage.sh')
    }
}'''.stripIndent())
    }
}