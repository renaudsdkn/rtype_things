# Whanos 🐋

## Description
Whanos est une plateforme d'intégration continue (CI) et de déploiement automatisé basée sur Jenkins et Kubernetes. Elle permet de construire et déployer automatiquement des applications dans différents langages de programmation en utilisant des images Docker standardisées.

## Technologies supportées
- C
- Java
- JavaScript/TypeScript
- Python
- Befunge

## Prérequis
- Docker
- Jenkins
- Kubernetes (Minikube)
- Git

## Architecture

### Images Docker

Whanos fournit deux types d'images Docker pour chaque langage supporté :

- **Images de base** : Utilisées comme point de départ pour la construction d'applications
- **Images standalone** : Pour le déploiement direct d'applications

### Structure des images

images/
├── befunge/
│ ├── Dockerfile.base
│ └── Dockerfile.standalone
├── c/
├── java/
├── javascript/
└── python/

## Configuration

### Jenkins
Le fichier `job.groovy` configure automatiquement Jenkins avec :
- Un dossier pour les images de base Whanos
- Un dossier pour les projets
- Des jobs de construction pour chaque langage supporté
- Un job de liaison pour les nouveaux projets

### Kubernetes
Le déploiement sur Kubernetes est géré via :
- `whanos.sh` : Script de déploiement
- `whanos.yml` : Configuration du déploiement

## Utilisation

### Construction d'images de base

#### Construction de toutes les images de base
jenkins-job-trigger "Whanos base images/Build all base images"

#### Construction d'une image spécifique
jenkins-job-trigger "Whanos base images/whanos-{language}"


### Déploiement d'un projet

1. Créez un fichier `whanos.yml` dans votre projet
2. Utilisez le job "link-project" dans Jenkins avec :
   - GITHUB_REPOSITORY_URL : URL de votre dépôt
   - DISPLAY_NAME : Nom d'affichage du projet

### Configuration du déploiement

Exemple de `whanos.yml` :
```yaml
deployment:
  replicas: 2
  resources:
    limits:
      memory: "128Mi"
      cpu: "500m"
    requests:
      memory: "64Mi"
    ports:
      - containerPort: 80
```