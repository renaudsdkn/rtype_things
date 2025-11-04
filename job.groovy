folder("Whanos base images") {
 displayName("Whanos base images")
 description("Basic whanos images.")
}

folder("Projects") {
 displayName("Projects")
 description("Projects available on whanos.")
}

languages = ["c", "java", "javascript", "python", "befunge"]

languages.each { language ->
 freeStyleJob("Whanos base images/whanos-$language") {
  steps {
   shell("docker build /images/$language -f /images/$language/Dockerfile.base -t whanos-$language")
   shell("docker tag whanos-$language localhost:5000/whanos-$language")
   shell("docker push localhost:5000/whanos-$language")
   shell("docker pull localhost:5000/whanos-$language")
   shell("docker rmi whanos-$language")
  }
 }
}

freeStyleJob("Whanos base images/Build all base images") {
 publishers {
  downstream(
   languages.collect { language -> "Whanos base images/whanos-$language" }
  )
 }
}

freeStyleJob('link-project') {
    parameters {
        stringParam('GITHUB_REPOSITORY_URL', "", 'Git URL to link')
        stringParam('DISPLAY_NAME', "", 'Display name for the job')
    }
    steps {
        dsl {
            text(''' freeStyleJob("Projects/$DISPLAY_NAME") {
                wrappers {
                    preBuildCleanup()
                }
                scm {
                    git {
                        remote  {
                            name('main')
                            url("$GITHUB_REPOSITORY_URL")
                            credentials('github-key')
                        }
                    }
                }
                triggers {
                    scm("* * * * *")
                }
                steps {
                    shell('/var/jenkins_home/whanos.sh $DISPLAY_NAME')
                }
            }'''.stripIndent())
        }
    }
}