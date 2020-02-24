---
layout: post
title: "Kubernetes on Debian Buster"
date: 2019-08-28 09:00:00  +0200
categories: linux debian virt
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
# kube_proxy_strict_arp: true
vi inventory/univention/group_vars/k8s-cluster/addons.yml
# helm_enabled: true
# metrics_server_enabled: true
# metrics_server_kubelet_insecure_tls: true
# metrics_server_kubelet_preferred_address_types: "InternalIP"

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

(see `contrib/misc/clusteradmin-rbac.yml`)

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
	wget --no-check-certificate wget https://nissedal.knut.univention.de/ucs-root-ca.crt
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
helm list
helm upgrade -f values.yaml gitlab-runner gitlab/gitlab-runner --version 0.14.0
helm upgrade -f values.yaml gitlab-runner ~/REPOS/VIRT/gitlab-runner

cd ..
```

# Issues

## Missing Service account

> ERROR: Job failed (system failure): pods is forbidden: User "system:serviceaccount:default:default" cannot create resource "pods" in API group "" in the namespace "default"

Enable `rbac/create=true` in `values.yaml` for `helm` to create the role automatically.

## Docker image not pulled

> ERROR: Job failed: image pull failed: Back-off pulling image "docker-registry.knut.univention.de/phahn/ucs-minbase:latest"

Missing SSL CA certificate on Host system, where `dockerd` tries to pull the
image.

```bash
cd /usr/local/share/ca-certificates
[ -s ucs-too-ca-.crt ] ||
	wget --no-check-certificate wget https://nissedal.knut.univention.de/ucs-root-ca.crt
update-ca-certificates
systemctl restart docker.service
```

## Dashboard

You need a *bearer token*, which you can retrieb via `kubectl`:

(see `contrib/misc/clusteradmin-rbac.yml`)

```bash
kubectl get clusterRoleBindings
# ...
# tiller                                                 110d
# tiller-admin                                           110d
kubectl describe serviceaccount tiller -n kube-system
# Mountable secrets:   tiller-token-wshmm
# Tokens:              tiller-token-wshmm
kubectl describe secret tiller-token-wshmm -n kube-system
# token:      ...
```

## Load balancer

By default k8s does not provide a load balaner implementation:
Many cloud providers provide that service out-of-the-box.
If k8s runs on bare-metal servers or in your own virtual machines, you must provide that service manually.
One option is to use [MetalLB](https://metallb.universe.tf/).

```bash
ansible-playbook \
	-i inventory/univention/hosts.yml \
	--become --become-user=root \
	-e metallb.ip_range=10.200.17.200-10.200.17.216 \
	contrib/metallb/metallb.yml
```

## Infinite firewall rule spamming

Because of [Issue 82361](https://github.com/kubernetes/kubernetes/issues/82361) `k8s` adds new firewall rules to the `DROP` table, which slows down the system.
For Debian Buster the `iptables` program must be switched back to the legacy version:

	update-alternatives --set iptables /usr/sbin/iptables-legacy)
	iptables -F DROP

# Single node cluster and upgrades

`kubespray` fails to upgrade a single node cluster as it drains and cordons the single node.
Essential services like `CoreDNS` are then no longer running and the update fails in `roles/kubernetes/master/tasks/kubeadm-upgrade.yml`.

Also see [kubeadm upgrade](https://kubernetes.io/docs/tasks/administer-cluster/kubeadm/kubeadm-upgrade/),

1. Update `inventory/univention/group_vars/k8s-cluster/k8s-cluster.yml`:

		kube_version: v1.16.7

2. Download new binaries and images:

		ansible-playbook -b -i inventory/univention/hosts.yml cluster.yml --tags=download -e download_run_once=true -e download_localhost=true -D

3. Upgrade all components

		ansible-playbook -b -i inventory/univention/hosts.yml cluster.yml --tags=docker
		ansible-playbook -b -i inventory/univention/hosts.yml cluster.yml --tags=etcd
		ansible-playbook -b -i inventory/univention/hosts.yml cluster.yml --tags=vault
		ansible-playbook -b -i inventory/univention/hosts.yml cluster.yml --tags=node --skip-tags=k8s-gen-certs,k8s-gen-tokens
		ansible-playbook -b -i inventory/univention/hosts.yml cluster.yml --tags=master
		ansible-playbook -b -i inventory/univention/hosts.yml cluster.yml --tags=client
		ansible-playbook -b -i inventory/univention/hosts.yml cluster.yml --tags=network
		ansible-playbook -b -i inventory/univention/hosts.yml cluster.yml --tags=apps
		ansible-playbook -b -i inventory/univention/hosts.yml cluster.yml --tags=helm

4. Upgrade k8s:

		kubeadm upgrade apply v1.16.7
