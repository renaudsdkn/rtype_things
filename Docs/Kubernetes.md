# 🎮 Guide Kubernetes pour Whanos

## 🌟 Introduction

Bienvenue dans le monde magique de Kubernetes avec Whanos !
Imaginez Kubernetes comme un chef d'orchestre et vos applications
comme des musiciens. Notre configuration permet d'harmoniser tout ce petit monde.

## 🎯 Architecture Whanos sur Kubernetes

### 🏗️ Structure de Base
```
whanos/
├── whanos.yml     # Configuration des déploiements
└── whanos.sh      # Script d'orchestration
```

### 🔄 Flux de Déploiement
```mermaid
graph LR
    A[whanos.sh] --> B[Initialisation Cluster]
    B --> C[Vérification Nœuds]
    C --> D[Déploiement Application]
    D --> E[Vérification Pods]
```

## 🚀 Configuration du Cluster

### 📋 Prérequis Minimaux
- 2 nœuds Kubernetes (obligatoire)
- Minikube installé et configuré
- kubectl configuré

### ⚙️ Configuration Standard
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: whanos
spec:
  replicas: 2              # Nombre minimal de réplicas
  resources:
    limits:
      memory: "128Mi"      # Limite mémoire par pod
      cpu: "500m"         # Limite CPU par pod (500 millicores)
```

## 🎯 Commandes Essentielles

### 🏃‍♂️ Démarrage Rapide
```bash
# Lancement du déploiement
./whanos.sh

# Vérification du statut
kubectl get pods -l app=whanos
```

### 🔍 Surveillance
```bash
# Voir les logs en temps réel
kubectl logs -f -l app=whanos

# Vérifier la santé des nœuds
kubectl get nodes
```

## 🎨 Personnalisation

### 🛠️ Variables Configurables dans whanos.yml
| Variable | Description | Valeur par défaut |
|----------|-------------|-------------------|
| replicas | Nombre de copies | 2 |
| memory | Limite mémoire | 128Mi |
| cpu | Limite CPU | 500m |
| port | Port conteneur | 80 |

### 🎪 Exemple de Configuration Avancée
```yaml
spec:
  replicas: 3
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0
```

## 🚨 Dépannage

### 🔬 Diagnostics Courants
1. **Les pods ne démarrent pas**
   ```bash
   kubectl describe pod <pod-name>
   ```

2. **Cluster inaccessible**
   ```bash
   minikube status
   minikube start --nodes 2
   ```

### 🎯 Codes d'Erreur Communs
| Code | Description | Solution |
|------|-------------|----------|
| ImagePullBackOff | Image introuvable | Vérifier le registry |
| CrashLoopBackOff | Application crash | Vérifier les logs |
| Pending | Pod en attente | Vérifier les ressources |

## 🌈 Bonnes Pratiques

### ✅ À Faire
- Toujours utiliser des limites de ressources
- Configurer des healthchecks
- Utiliser des labels appropriés

### ❌ À Éviter
- Déployer sans replicas
- Ignorer les logs
- Oublier de vérifier l'état du cluster

## 🎉 Astuces Pro

1. **Déploiement Rapide**
   ```bash
   alias wh-deploy='./whanos.sh'
   ```

2. **Surveillance en Temps Réel**
   ```bash
   watch -n 1 'kubectl get pods -l app=whanos'
   ```

3. **Nettoyage Rapide**
   ```bash
   kubectl delete -f whanos.yml
   ```

## 🔗 Intégration avec Jenkins

Le déploiement Kubernetes s'intègre parfaitement avec Jenkins via :
- Déclenchement automatique après build
- Vérification du statut de déploiement
- Notification en cas d'échec

## 📊 Métriques et Surveillance

### 🔍 Métriques Importantes
- Utilisation CPU/Mémoire
- Temps de réponse
- Nombre de pods actifs

### 📈 Commandes de Surveillance
```bash
# Utilisation des ressources
kubectl top pods -l app=whanos

# État des pods
kubectl get pods -l app=whanos -w
```

## 🎓 Conclusion

Whanos sur Kubernetes offre une solution robuste et élégante
pour le déploiement d'applications. En suivant ce guide, vous
maîtriserez rapidement l'art du déploiement containerisé ! 
