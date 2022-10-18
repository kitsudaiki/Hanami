# Hanami-AI

![Logo](img/hanami-logo-with-text.png)


**This is the meta-repo for the Hanami-AI to handle global issues of the project and release-taggings.**

**IMPORTANT: This project is still an experimental prototype at the moment and NOT ready for productive usage.** 

There is still a huge bunch of known bugs and missing validations, which can break the backend. Even the documentation here is quite basic. Normally I absolutely dislike it to make something public, which has known bugs and other problems, but I simply don't wanted to wait longer for the open-sourcing of this project. Most of it will be fixed until [Version `0.2.0`](/#roadmap). Keep in mind, that this project is created by a single person in his spare time beside a 40h/week job. ;)

## Documentation

All of this page and more in the documentation on: 

https://docs.hanami-ai.com

## Intro

Hanami-AI is an AI-as-a-Service project, based on a concept created by myself. It is written from scratch with a Backend in C++ with Web-frontend.

The actual prototype consists of:

- partially implementation of an own concept for an artificial neuronal network. It has no fixed connections between the nodes, but creates connections over time while learning. Additionally it doesn't need a normalization of input-values and this way it can also handle unknown data as input. This should make it flexible and efficient. The current state is extremely experimental.
- multi-user- and multi-project-support, so multiple-users can share the same physical host
- basic energy-optimization supporting the scheduling of threads of all components and changing the cpu-frequency based on workload
- basic monitoring of cpu-load
- Webfrontend with client-side rendering and SDK-library
- Websocket-connection to directly interact with the artificial neuronal networks
- CI-pipelines, Test-Tool, Docker-build-process and basic helm-chart to deploy the project on Kubernetes

## First benchmark

Test-case:

- Dataset: MNIST handwritten letters
- Hardware: Intel i7-1165G7 and 16GB RAM with 3200MT/s
- Settings: 
    - **CPU** with **one processing thread** 
    - **no batches**, so each of image is processed one after the other
    - values are pushed directly into the network without normalization between 0 and 1
    - average of 10 measurements


|             |      average result        |
| ----------- | ------------------------------------ |
| time for train-dataset (60000 Images); 1. epoch  | 1.9 s |
| time for test-dataset (10000 Images)       |  0.1 s |
| correctness of test-dataset after 1. epoch   |  94.21 % |
| correctness of test-dataset after 10. epoch   |  96.43 % |


In an older version there was already a state, where up to *98,1%* were correct with a similar speed, but removing the limitation for the input-values to be able to handle much bigger values had its price, but I think this exchange was it worth. Some things are still missing in the implementation and there is still much space for optimization and research, so I think this is not the maximum possible at the moment.

## Possible use-case

Because the normalization of input is not necessary, together with the good performance of training single inputs (based on the benchmark) and the direct interaction remotely over websockets, could make this project useful for processing measurement-data of sensors of different machines, especially for new sensors, where the exact maximum output-values are unknown. So continuous training of the network right from the beginning would be possible, without collecting a bunch of data at first.

## Basics

- Installation-Guide to deploy HanamiAI on a kubernetes for testing:

    https://docs.hanami-ai.com/how%20to/1_installation/

- To get a first impression there is a first example-workflow via the dashboard:

    https://docs.hanami-ai.com/how%20to/2_dashboard/

- For the naming at some points look into the Glossar:

    https://docs.hanami-ai.com/other/2_glossar/

- Even it is quite basic for now, there are also some internal workflow and tasks of the single components described:

    https://docs.hanami-ai.com/inner%20workings/1_overview/

- Many basic dependencies were created in context of this project. Here is an overview of all involved repositories:

    https://docs.hanami-ai.com/other/1_dependencies/

## Core-components

for more details see: https://docs.hanami-ai.com/inner%20workings/1_overview/

- **Kyouko**
    - Content: Core-component, which holds the artificial neuronal networks.
    - Repository: https://github.com/kitsudaiki/KyoukoMind.git

- **Misaki**
    - Content: Authentication-service and management of user
    - Repository: https://github.com/kitsudaiki/MisakiGuard.git

- **Shiori**
    - Content: Storage-component, which holds snapshots, logs and so on
    - Repository: https://github.com/kitsudaiki/ShioriArchive.git

- **Azuki**
    - Content: Monitoring and energy-optimization
    - Repository: https://github.com/kitsudaiki/AzukiHeart.git

- **Torii**
    - Content: Proxy for all incoming connections
    - Repository: https://github.com/kitsudaiki/ToriiGateway.git


## Roadmap

- **0.1.0**
    - *content*: 
        - first prototype with basic feature-set

- **0.2.0**
    - *expected date*: end Q4 2022
    - *content*: 
        - no new features but only improving the current state with:
            - bugfixes
            - additional validation
            - more documentation
            - ...

- **0.3.0**
    - *expected date*: Q2 2023
    - *content*: 
        - complete implementation of the core-concept and further evaluation and improvement of the learning-process:
            - allow to use it as spiking-neuronal-network
            - remove strict layer-structure, which is still enforced by hard configuration at the moment
            - build 3-dimensional networks
        - further evaluation and improving of the core-process
        - add classical static neuronal networks with GPU-support

- **0.4.0**
    - *expected date*: Q4 2023
    - *content*: 
        - first Multi-Node-Setup


Most of the content of `0.3.0` and `0.4.0` already exist in threorie, but I had not the time to implement it until now. If I could work full-time on the project and not only in my spare time, then I could be much faster then this actual schedule. So if you highly interested in this project and want to push my progress, then feel free to hire me and pay me, so I can work much more for this project.

## Issue-Overview

[Hanami-AI-Project](https://github.com/users/kitsudaiki/projects/9/views/2)

## Author

Tobias Anker

eMail: tobias.anker@kitsunemimi.moe

## License

The complete project is under Apache 2 license.

## Contributing

I'm happy, if you find the project interesting enough to contribute code. In this case, please wait until version `0.2.0`, because there are many API- and Database-breaking changes on the project. Additionally until `0.2.0` I will also provide a Code-Styling-Guide (at least for the C++-backend).
