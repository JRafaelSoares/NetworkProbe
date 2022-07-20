 locals{
    depends_on = [grid5000_deployment.my_deployment1,grid5000_deployment.my_deployment2,grid5000_deployment.my_deployment3]

    nodes_list    = setunion(grid5000_deployment.my_deployment1.nodes, grid5000_deployment.my_deployment2.nodes, grid5000_deployment.my_deployment3.nodes)
    deployment_list = [grid5000_deployment.my_deployment1, grid5000_deployment.my_deployment2, grid5000_deployment.my_deployment3]

    deployment1 = [ for node in local.deployment_list[0].nodes : {
        address = node
        internal_address = node
        role = ["worker"]
        labels = {
          dc = local.deployment_list[0].site
        }
      }]

    deployment2 = [ for node in local.deployment_list[1].nodes : {
        address = node
        internal_address = node
        role = ["worker"]
        labels = {
          dc = local.deployment_list[1].site
        }
      }]

    deployment3 = [ for node in local.deployment_list[2].nodes : {
            address = node
            internal_address = node
            role = ["worker"]
            labels = {
              dc = local.deployment_list[2].site
            }
      }]
    joint_deployment = concat(local.deployment1, local.deployment2, local.deployment3)
}

output "deployment1_print" {
  value = local.deployment1
}

output "deployment2_print" {
  value = local.deployment2
}

output "deployment3_print" {
  value = local.deployment3
}

output "joint_deployment_print" {
  value = local.joint_deployment
}

resource "null_resource" "docker_install" {
  depends_on = [grid5000_deployment.my_deployment1,grid5000_deployment.my_deployment2,grid5000_deployment.my_deployment3]

  count = local.nodes_count


  connection {
    host          = element(sort(local.nodes_list), count.index) 
    type          = "ssh"
    user          = "root"
    private_key   = file("~/.ssh/id_rsa")
  }

  provisioner "file" {
    source = "install-docker.sh"
    destination = "/root/install-docker.sh"
  }

  provisioner "remote-exec" {
    inline = [
      "sh /root/install-docker.sh >/dev/null 2>&1",
    ]
  }
}

resource "rke_cluster" "cluster" {
  depends_on = [null_resource.docker_install, local.joint_deployment]

  dynamic "nodes" {

    for_each = [for s in range(length(local.joint_deployment)): {
      address = local.joint_deployment[s].address
      internal_address = local.joint_deployment[s].internal_address
      role = s > 0 ? ["worker"] : ["controlplane", "etcd", "worker"]
      labels = {
        dc = local.joint_deployment[s].labels.dc
        selector = element(local.types, s)
      }
    }]

    content {
      address = nodes.value.address
      internal_address = nodes.value.internal_address
      user = "root"
      role = nodes.value.role
      labels = {
        dc = nodes.value.labels.dc
        selector = nodes.value.labels.selector
      }
      ssh_key = file("~/.ssh/id_rsa") 
    }
  }
}

provider "kubernetes" {
  load_config_file        = false
  host                    = rke_cluster.cluster.api_server_url
  username                = rke_cluster.cluster.kube_admin_user

  client_certificate      = rke_cluster.cluster.client_cert
  client_key              = rke_cluster.cluster.client_key
  cluster_ca_certificate  = rke_cluster.cluster.ca_crt
}

resource "kubernetes_namespace" "monitoring" {
  metadata {
    name = "monitoring"
  }
}

