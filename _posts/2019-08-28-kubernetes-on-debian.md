---
layout: post
title: "Kubernetes on Debian Buster"
date: 2019-08-28 09:00:00  +0200
categories: linux debian
excerpt_separator: <!--more-->
---

This is some WIP.
Setup GitLab in Kubernetes (k8s) test cluster using the helm chart:

<!--more-->

# Setup k8s using kubespray

Setup a new cluster with 2 nodes:

```bash
cd ~/REPOS/VIRT

[ -d kubespray ] ||
	git clone https://github.com/kubernetes-sigs/kubespray.git ./kubespray
cd ./kubespray

[ -d inventory/univention ] ||
	cp -r inventory/sample inventory/univention

declare -a IPS=(10.200.17.217 10.200.17.218)
CONFIG_FILE=inventory/univention/hosts.yml \
	python3 contrib/inventory_builder/inventory.py "${IPS[@]}"

vi inventory/univention/group_vars/all/all.yml
vi inventory/univention/group_vars/k8s-cluster/k8s-cluster.yml
# kubeconfig_localhost: true
# kubectl_localhost: true
vi inventory/univention/group_vars/k8s-cluster/addons.yml
# helm_enabled: true

ansible-playbook \
	-i inventory/univention/hosts.yml \
	--become --become-user=root \
	cluster.yml

kubectl -n kube-system get deployments
cd ..
```

# Configure k8s for GitLab

(This is from <https://docs.gitlab.com/ee/user/project/clusters/>):

Get the k8s **API URL**:

```bash
kubectl cluster-info |
	awk '/Kubernetes master.*http/ {print $NF}'
```

Get the **CA certificate**:

```bash
kubectl get secrets
kubectl get secret default-token-<secret name> -o jsonpath="{['data']['ca\.crt']}" |
	base64 --decode
```

Create **cluster-admin** account:

1. Create a file called `gitlab-admin-service-account.yaml` with contents:

	```yaml
apiVersion: v1
kind: ServiceAccount
metadata:
  name: gitlab-admin
  namespace: kube-system
---
apiVersion: rbac.authorization.k8s.io/v1beta1
kind: ClusterRoleBinding
metadata:
  name: gitlab-admin
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: cluster-admin
subjects:
- kind: ServiceAccount
  name: gitlab-admin
  namespace: kube-system
```

2. Apply the service account and cluster role binding to your cluster:

	```bash
kubectl apply -f gitlab-admin-service-account.yaml
```

3. Retrieve the token for the `gitlab-admin` service account

	```bash
kubectl -n kube-system describe secret \
	$(kubectl -n kube-system get secret | awk '/gitlab-admin/{print $1}') |
	sed -ne 's/^token: *//p'
```

# Setup GitLab runner

Create a customized GitLab runner, which includes our custom SSL certificate used by our internal Docker registry.
Due to [GitLab Issue 3968](https://gitlab.com/gitlab-org/gitlab-runner/issues/3968) we also have to setup the same certificate for the Runner to be able to access the GitLab master instance.

```bash
[ -f gitlab-runner ] ||
	https://gitlab.com/charts/gitlab-runner.git/
cd gitlab-runner

[ -s ucs-too-ca-.crt ] ||
	wget wget https://nissedal.knut.univention.de/ucs-root-ca.crt
kubectl create secret generic ca --from-file=ucs-root-ca.crt

vi values.yaml
# gitlabUrl: https://git.knut.univention.de/
# runnerRegistrationToken: "XXXXXXXXXXXXXXXXXXXX"
# certsSecretName: ca
# rbac:
#   create: true
# runners:
#   image: docker-registry.knut.univention.de/phahn/ucs-minbase:latest
#   imagePullPolicy: "always"
#   locked: false
#   tags: "docker"
#   privileged: true
# envVars:
#   - name: CI_SERVER_TLS_CA_FILE
#     value: /home/gitlab-runner/.gitlab-runner/certs/ucs-root-ca.crt
#   - name: CONFIG_FILE
#     value: /home/gitlab-runner/.gitlab-runner/config.toml
#   - name: REGISTER_RUN_UNTAGGED
#     value: true

helm repo add gitlab https://charts.gitlab.io
helm init --client-only
helm install --name gitlab-runner -f values.yaml gitlab/gitlab-runner
helm status gitlab-runner

# NAMESPACE='kube-system'
helm upgrade -f values.yaml gitlab-runner gitlab/gitlab-runner

cd ..

# ERROR: Job failed (system failure): pods is forbidden: User "system:serviceaccount:default:default" cannot create resource "pods" in API group "" in the namespace "default"
kubectl create role pod-reader --verb=get,list,watch --resource=pods -n default
kubectl create rolebinding sa-read-pods --role=pod-reader --user=system:serviceaccount:default:default-service-account -n default
```
