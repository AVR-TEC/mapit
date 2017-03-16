# mapit

## Overview 

Mapit is designed to offer easy communication between multiple participants for accessing and editing sensordata. This is archieved by following design principles of distributed versioning systems and algorithm pipelines (workflows).

For example  the following shell script creates a new repository locally (*.mapit* directory).
It creates a new *checkout*. Note, that *checkouts*, in contrast to other versioning systems, are not visible in the filesystem (yet).
Data is read into the checkout and is edited with *execute_operator* command. In the end data is made visible to the filesystem again (*export2filesystem*).

	mapit create_checkout testcheckout master
	mapit execute_operator testcheckout load_pointcloud '{"filename":"./data/bunny.pcd", "target":"testmap/testlayer/bunny"}'
	mapit execute_operator testcheckout voxelgridfilter '{"leafsize":0.2, "target":"testmap/testlayer/bunnyVoxelgrid"}'
	mapit execute_operator testcheckout normalestimation '{"radius":0.2, "target":"testmap/testlayer/bunnyNormalEst"}'
	mapit checkout2filesystem testcheckout ./export

## Features
Mapit is meant to be used with huge files while still maintaining the history of data. This requires the use of new paradigms.

**Distributed access and computation**

Data managed with mapit can be distributed across multiple *repositories*. When executing algorithms on data, the download/upload of huge amounts of data can be overcome and calculation can be done in-place. At the same time there is no need to repeatedly store all data in every repository.

<img src="doc/repositories.svg" width="750">

**Mapit tracks Metadata**

Sensordata is not (always) copied and stored in history once the data is changed. By keeping a description of the executed algorithms (Metadata) it ensures that snapshots from the past can be recovered at any time. This comes with the downside, that data must never be changed by the user directly. Thus, data is not visible as editable files in the filesystem. Data can be changed by existing or self implemented operators in the system.

<img src="doc/workflow.svg" width="500">

Every executed operation is described by one chunk of metadata. A group of operations, executed in sequence is called a *workflow*. With mapit its possible to group several operations together into one workflow which then can be easly applied onto new sensoric data. (This is not yet supported/implemented).

**Extensible in multiple ways**

For developers mapit can be extended in 3 ways.

- It is easy to add **new operators**, by using an CMake-Template we provide. You implement a function that receives a simple C++-Class wich enables reading/writing to data. The functions receives a string which can contain more user defined parametrization in any string format (e.g. XML, YAML, JSON).
- If the provided layertypes (PCD, OVDB, LAS, ...) are not enough, **new layertypes** can be implemented by writing a C++-class that serializes your custom data to/from a std::stream. Also serialization directly to/from files or memoryadresses is possible to boost performance in certain scenarios (e.g. network access), but these are optional.
- **Tools** (e.g. for visualization, merging, browsing, ...) can be implemented by using mapit as a library. When used as a library the full power of mapit is exposed to you, featuring e.g. versioning access. This is only meant for advanced use cases.

We identified that the most common extension will be to add more operators. Consequently we made the interface for new operators as easy to use as possible.

## Usage	(for non-developers)

To use mapit to access and edit data there is no need to develop a line of code.

### Basics

All commands of mapit are accessible through the command

	mapit <command> [<args>] [--repository-directory| --url] [--compute-local]

All commands work on repositories. A repository can be either a local directory (usually called ".mapit") or a network address.

- *\-\-repository-directory* uses a local repository,
- *\-\-url* specifies a network repository.

This is one of the few places where the user has to know about the place of a repository. For the most time it is transparent - meaning it makes no difference - where a repository resides. The software handles network access (if neccesary) without futher contribution of the user. If none of the two paramters is given, the local repository at "./.mapit" will be used/created. Examples:

	mapit <cmd> <args> --repository-directory ~/repositories/mymaps
	mapit <cmd> <args> --url tcp://nucular.local:55555

The *\-\-compute-locale*-Flags specifies, that all computation has to take place on the local machine. If the repository is a remote one, all data is downloaded and uploaded. If the falg is not specified the default behaviour is that the computer with the repository (e.g. the computer that runs maptitd) does the calculation. Different repositories may have differten operators installed. For example there might be a Microsoft Windows machine, witch has no version of PCL on it. A person using this computer can trigger the execution of a PCL algorithm on a remote machine.

Currently there is a small list of *commands*:

	checkout_create
	execute_operator
	mapitd
	checkout2filesystem
	
(TODO: the names are about to change in order to create a streamlined interface)

### Read Data into the software

Getting data into the system is done by using a special operator, which accesses the filesystem it is running on. For Pointlocuds the operator is calls *load_pointcloud*

	mapit execute_operator testcheckout load_pointcloud '{"filename":"./data/bunny.pcd", "target":"testmap/testlayer/bunny"}'

### Write Data to filesystem

All data is hidden from the user by default to prevent accidential edits of the data. To track all changes it is important that all changes are done via operators (and in the best case can be represented as metadata). In later versions it might be possible to create read-only symlinks to checked-out files.

There is a tool which extracts/copies a complete checkout to a directory:

	mapit checkout2filesystem <checkout> <destination>
	
For example:

	mapit checkout2filesystem testcheckout ./export

### Edit data

Data can only be changed by operators. Operator always work on a specific version of the data, which is represented as a *checkout*. There are no operator that works across multiple versions of data.
Executing an operator is possible with:

	mapit execute_operator <checkout> <operator> <params>
	
The list of operators is always growing, currently these operators are available/in-development

#### centroid_to_origin
- Requires: PCL
- Parameters: JSON
    - target: input and output
- Effect: Removes offset from a pointcloud and tries to reposition it at the center.
- Algorithm: Calculates a axisaligned boundingbox and demeans the pointcloud using the BBs center.

#### copy

- Requires: - 
- Parameters: JSON
    - target: input and output
- Effect: 
- Algorithm:

#### grid (nyi)

- Requires: -
- Parameters: JSON
    - target: input and output
    - leafsize: length of the side of each grid cell
    - cells: overrides leafsize; number of cells to create
- Effect: Splitting the pointcloud into qubes, where each qube results in an new pointcloud containing the points of the original pointcloud that are within this qube.
- Algorithm: Not yet implemented

#### levelset_to_mesh

- Requires: PLY Asset, OpenVDB
- Parameters: JSON
    - target: input and output; overrides input/output
    - input: ovdb levelset entitiy
    - output: surface as PLY model
    - detail: adaptivity of the mesh; between 0.0 and 1.0 with 0.0 beeing the highest detail level.
- Effect: Creates a Polygon model out of an OpenVDB Levelset
- Algorithm: uses OpenVDB Algorithm

#### load_pointcloud

- Requires: PCL
- Parameters: JSON
    - target: output entity
    - filename: filename from the machine, the operator is running on
- Effect: loads a pointcloud
- Algorithm: pcl::Pointcloud2 is used internally. This can represent all Pointtypes. However, operators will only work on specific point types (e.g. PointXYZNormal, PointXY). The type of Pointcloud that was read by the operator is not known by the system.

#### normalestimation

- Requires: PCL
- Parameters: JSON
    - target: input and output
    - radius: radius to search for neighbours in
- Effect: Generates normals for a pointcloud
- Algorithm: Uses RadiusSearch algorithm of PCL.

#### ovdb_smooth

- Requires: OpenVDB
- Parameters: JSON
    - target: input and output
    - radius: distance to blow the surface up and shrink back
    - smoothness: radius for gaussian blur
- Effect: Inflates the surface (might be usefull to close holes). Then the surface is smoothed using gaussian filter. At last the surface is deflated again to it's original volume.
- Algorithm: OpenVDB LevelSetFilter. 1) filter.offset(-radius), 2) filter.gaussian(smoothness), 3) filter.offset(radius)

#### surfrecon_openvdb

- Requires: PCL, OpenVDB
- Parameters: JSON
    - target: input and output; overrides input/output
    - input: pcl pointcloud
    - output: surface as ovdb levelset
    - radius: radius of the created spheres. should be larger than the avg. distance between points and as small as possible.
    - voxelsize: 
- Effect: Reconstructs a surface from a pointcloud
- Algorithm: Creates level-set-spheres for each point

#### transform (nyi)

- Requires: tf (eigen3)
- Parameters: JSON
    - target: input and output
- Effect: Creates a transform
- Algorithm:

#### voxelgridfilter

- Requires: 
- Parameters: JSON
    - target: input and output
    - leafsize: size of each cell
- Effect:  Thin out a pointcloud. Each voxel of size <leafsize\> will contain one or zero points.
- Algorithm: 

Operators are versioned, the actually executed version is stored in metadata.

### Access remote Repositories

See **Usage Basics** to see how \-\-compute-local flag works.
Downloading und uploading data is done by configuring the repository to the remote by using the *--url* flag.
For downloading use

	mapit export2filesystem <checkout> <local_destination_directory> --url tcp://<ip|hostname>:<port>

To upload pointclouds to a remote repository use:

	mapit execute_operator <checkout> load_pointcloud '{"filename":"<filename>", "target":"<name_of_new_entity>"}'

### Create an remote accesible Repository


## Architecture
Mapit is designed in a modular way. 


## Artifacts
Mapit is delivered as a shared library and a set of executables. Public headers are included for development. 
This makes it ready to use from the commandline and extensible.

We also provide a docker container which takes care of the installation of the dependencies witch would be hard to configure otherwise.

### Binaries:

- bin/mapit: executable that servers for an entrypoint for other mapit-tools
- bin/upns_tools: tools and aliases. This contains basically the first argument of the *mapit* command.


### Libaries

- lib/libupns_mapmanager.so: shared library containing symbols for all extensions. This is the core of mapit.
- lib/libstandard_repository_factory.so: library used by tools to create C++-Classes to access and manipulate *Repositories* and *Checkouts*.
- lib/upns_layertypes/*: types of sensordata the system can work with. For example Pointclouds (PCL).
- lib/upns_operators/*: each shared library represents an algorithm that can be executed on the data.

### Headers

**common headers**

- include/upns/upns_layertypes/*: Headers for concrete layertypes. Also the corresponding shared library of a concrete layertype must be linked. Note that most layertypes come with dependencies which must be satisfied (e.g. PCL). Only needed layertypes should be included into a project. Concrete layertypes are needed for **operators** and **tools**, but very unlikely for other layertypes.



**for implementing new operators**
**for implementing new layertypes**



**for implementing new tools**

- Repository Factories:
-- include/upns/versioning/repositoryfactorystandard.h: Header for commandline tools. This is only needed for new *tools*.
-- include/upns/versioning/repositoryfactory.h: Header to create a local directory repository. This is only needed for new *tools*.
-- include/versioning/repositorynetworkingfactory.h: Header to create a remote repository that always uses network. This is only needed for new *tools*.


- include/upns/upns_operators/*: Headers for new algorithms. This is only needed for new *operators*.

**Docker Container**

- server/mapitd
- helper (TODO)

### Checkout

For basic usage it is only required to understand the concept of *checkouts*. Details about versioning can be ignored, when working only with one checkout. When using git as an analogy, this is like working with an directory tree, completely forgetting about verioning.
A checkout is nothing more than a directory tree. In mapit, folders are called 'tree' (at the moment) and 'entities', which represent files.
A checkout is one specific version of all the data you are currently working on.
In contrast to git and other versioning systems, a checkout has name in mapit. There can be multiple checkouts and checkouts might also exist on remote machines.

A checkout can only be accessed by mapit tools or by writing a new tool (see *using mapit as a library*). Usually tools only **read** the structure.
**Writing** is meant to be done by executing operators.

### Repository

If it is required to access the history of the data, a checkout is not enough. A repository gives you access to existing checkouts. Moreover you can create a checkout from version which existed in the past.
These version 


## Extending UPNS-Software
