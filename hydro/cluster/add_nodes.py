#!/usr/bin/env python3

#  Copyright 2019 U.C. Berkeley RISE Lab
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#  Modifications copyright (C) 2021 Taras Lykhenko, Rafael Soares

import random
import os


from hydro.shared import util


# Generate list of all recently created pods.
def get_current_pod_container_pairs(pods):
    pod_container_pairs = set()
    for pod in pods:
        pname = pod.metadata.name
        for container in pod.spec.containers:
            cname = container.name
            pod_container_pairs.add((pname, cname))
    return pod_container_pairs

def add_nodes(client, apps_client, cfile, kinds, counts, create=False,
              prefix=None):
    previously_created_pods_list = []
    expected_counts = []
    for i in range(len(kinds)):
        print('Adding %d %s server node(s) to cluster...' %
              (counts[i], kinds[i]))

        pods = client.list_namespaced_pod(namespace=util.NAMESPACE,
                                          label_selector='role=' +
                                          kinds[i]).items
        print('stuck')


        previously_created_pods_list.append(get_current_pod_container_pairs(pods))

        prev_count = util.get_previous_count(client, kinds[i])
        expected_counts.append(counts[i] + prev_count)


    for i in range(len(kinds)):
        kind = kinds[i]

        # Create should only be true when the DaemonSet is being created for the
        # first time -- i.e., when this is called from create_cluster. After that,
        # we can basically ignore this because the DaemonSet will take care of
        # adding pods to created nodes.
        if create:
            fname = 'yaml/ds/%s-ds.yml' % kind
            yml = util.load_yaml(fname, prefix)

            for container in yml['spec']['template']['spec']['containers']:
                env = container['env']

                #util.replace_yaml_val(env, 'ROUTING_IPS', route_str)
            yml['spec']['replicas'] = counts[i]
            if(counts[i] > 0):
                apps_client.create_namespaced_replica_set(namespace=util.NAMESPACE,
                                                          body=yml)
        else:
            util.scale_replica_set(apps_client, 'role='+kind, expected_counts[i])
        # Wait until all pods of this kind are running
        res = []
        while len(res) != expected_counts[i]:
            res = util.get_pod_ips(client, 'role='+kind, is_running=True)

        # base code if needed to add config file
        #pods = client.list_namespaced_pod(namespace=util.NAMESPACE,
        #                                  label_selector='role=' +
        #                                  kind).items

        #created_pods = get_current_pod_container_pairs(pods)

        #new_pods = created_pods.difference(previously_created_pods_list[i])

        # Copy the KVS config into all recently created pods.
        #os.system('cp %s ./anna-config.yml' % cfile)

        #for pname, cname in new_pods:
        #    if kind != 'function' and kind != 'gpu':
        #        util.copy_file_to_pod(client, 'anna-config.yml', pname,
        #                              '/hydro/anna/conf/', cname)
        #    else:
        #        if cname == 'cache-container':
        #            # For the cache pods, we also copy the conf into the cache
        #            # conf directory.
        #            util.copy_file_to_pod(client, 'anna-config.yml', pname,
        #                                  '/hydro/anna-cache/conf/', cname)

        #os.system('rm ./anna-config.yml')

def batch_add_nodes(client, apps_client, cfile, node_types, node_counts, batch_size, prefix):
  if sum(node_counts) <= batch_size:
    add_nodes(client, apps_client, cfile, node_types, node_counts, True,
              prefix)
  else:
    for i in range(len(node_types)):
        if node_counts[i] <= batch_size:
            batch_add_nodes(client, apps_client, cfile, [node_types[i]], [node_counts[i]], batch_size, prefix)
        else:
            batch_count = 1
            print('Batch %d: adding %d nodes...' % (batch_count, batch_size))
            add_nodes(client, apps_client, cfile, [node_types[i]], [batch_size], True,
                      prefix)
            remaining_count = node_counts[i] - batch_size
            batch_count += 1
            while remaining_count > 0:
              if remaining_count <= batch_size:
                print('Batch %d: adding %d nodes...' % (batch_count, remaining_count))
                add_nodes(client, apps_client, cfile, [node_types[i]], [remaining_count], False,
                          prefix)
                remaining_count = 0
              else:
                print('Batch %d: adding %d nodes...' % (batch_count, batch_size))
                add_nodes(client, apps_client, cfile, [node_types[i]], [batch_size], False,
                          prefix)
                remaining_count = remaining_count - batch_size
              batch_count += 1
