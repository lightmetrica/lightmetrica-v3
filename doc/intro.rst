Introduction
############

About this project
==================

.. General introduction. Briefly describe the purpose of this project.

**Lightmetrica** is a research-oriented renderer. The development of the framework is motivated by the goal to provide a practical environment for rendering research and development, where the researchers and developers need to tackle various challenging requirements through the development process.

.. Background and motivation of this project.

Unlike many other renderers, Lightmetrica is *not* a stand-alone renderer, but an *environment* to support entire process of renderer development. Stand-alone renderers are typically used by artists and its development focuses on the usability for artists and the performance to fully utilize the limited time or budgets of a production. On the other hand, research-oriented renderer is mainly utilized by researchers and renderer developers, to prototype new approaches where the developer needs to extend the renderer and setup various experiments utilizing the extended renderer. Moreover, research-oriented renderer needs to be verifiable, meaning the features must be correctly implemented and ready to be comparable.

Characteristics
===============

In particular, we can characterize the requirements of the research-oriented renderer with three criteria: *extensibility*, *verifiability* and *orchestration*. Lightmetrica provides various features to meet the requirements.

Extensibility
-------------

*Extensibility* allows the developer to extend the renderer and to implement a new feature. This is necessary as a research-oriented render because the developers want to focus on only the modification they really need. For instance, the developer who want to implement new materials doesn't want to modify the code of the rendering techniques.

Lightmetrica provides decoupled interfaces to extend various features so that the developers are not necessary to know the details on the other part of the renderer. Also, our plugin system enables to extend the feature extremely simple by adding one macro at the end of the C++ code. Moreover, our plugin system does not care about the number of implementations inside a library and the places where the implementation exists. 

Verifiability
-------------

*Verifiability* helps the developers to assure the extended feature to be correct. The process of rendering research is a sequence of trials-and-errors where a developer need to feedback the knowledge from the previous iteration to the next. Verifiability is crucial in renderer development because a developer need to know what is actually happening inside the code to feedback the knowledge. That is, a developer is responsible for giving a rationale to the phenomena they observed. For instance, if a developer found an inconsistency in an experiment, e.g., mismatch of the rendered vs. references, they need to identify where the cause of inconsistency comes from. It is crucially important to identify the phenomena is due to a bug or not, because otherwise a developer cannot have a clue to improve the approach.

Our framework provides tools to help developers to find the cause behind the observation as soon and as easy as they can, such as a visual debugger, tools to find statistical inconsistencies or to analyze performance of the code. Also, we provide various implementation with least possibility of bugs inside. Developers can utilize the code as a reference to find inconsistency in their codes.

Orchestration
-------------

*Orchestration* allows the developers to design and organize various experiments related to rendering research. Implementing a new technique is just one step in the research process. In addition to the implementation, a developer needs to design and implement various experiments to find characteristics of the technique. 

Our framework provides a comprehensible solution to design various experiments in Python, where we provide a complete Python API of every features including the extension of framework. As a result, we could successfully facilitate a richness of Python ecosystem. Internal types are automatically converted to Python types that compatible with Python's standard datatypes or Numpy's array type so that we can easily integrate inputs or outputs into the pipeline using other useful Python libraries. Also, we provide a IPython extension which supports us to implement various experiments interactively inside the Jupyter notebook.

Although it is still possible to use stand-alone renderers in the experiments, it requires another layer of API to manage input/output of the renderer where we usually needs constant maintenance as the renderer specification change (and it happens a lot for research purpose, by design). On the other hand, our framework exposes internal features directly to the developers, which enables broader applicability to the experiments. For instance, we can design an experiment where the data is shared among renderings with different parameters. This is obviously not possible with stand-alone renderers because we need to dispatch another process to execute the renderer where additional data transfer is unavoidable. 

Renderer as a library
--------------------------

In the previous version of the framework, all features of the renderer must be accessed through an executable. The parameters to the renderer must be passed by a scene definition file written in YAML format, and the outputs from the renderer must be written onto the disk. This execution model works great if the renderer used as an independent renderer. In a research project, however, a renderer is usually integrated into the workflow of experiments typically written in a scripting language, for instance to organize various input parameters and to analyze the outputs from the renderer. 

After the experience of various research projects, we found a renderer as an independent executable is rather suboptimal in this workflow. First, due to the separated layers of renderer IO, the experimental code needs to generate the configuration on the fly. This requires the user to maintain a layer to serialize input parameters to the renderer as a specific scene configuration format, which needs to be managed throughout the experiments. In the previous versions, we have used a parameterized scene definition file and generated the scene definition file on the fly where the scene definition can contain arbitrary parameters to the extended features. A problem is managing this intermediate layer is error prone and it becomes hard to manage as the research progresses. Once we modify the parameters, we need to modify both of the scene file generation and experimental codes. In the early stage of the development, it is not a problem, but later it becomes a dept since the size of the parameters being managed becomes larger and larger. 

Second, with an independent process, it is hard to conduct a series of experiments that utilizes persistent data among the experiments. For instance, let us consider an experiment to conduct a series of rendering jobs with several different parameters against the same scene. Naturally, we want to use the scene data as a persistent date among the executions to save scene loading time. Unfortunately, in the previous versions, it is not possible because we always need to restart the renderer process on every executions. 

To resolve the aforementioned problem, we decided to redesign the renderer as as a library. In Lightmetrica Version 3, all operations to the internal state of the renderer, such as modifying the scene or organizing rendering jobs, must be conducted by a script with provided API, not by a scene definition file. As a matter of course, the scene definition file is deprecated in Version 3. This decision is based on the observation of the requirements in the actual research environment where the renderer is mostly used with experimental codes. This modification could remove the necessity of the intermediate IO layers to communicate with a renderer process, which eventually contributed to the simpleness of the experimental code. 

Features
==========================

.. TODO: add link

Current major version is **3**. Upon the major update, we have redesigned most of the features from the previous version, and rewrote the entire framework from scratch.

- Extension support
    - Component object model that allows to extend any interfaces as plugins
    - Serialization of component objects
    - Position-agnostic plugins
- Verification support
    - Visual debugger
    - Performance measurements
    - Statistical verification
- Orchestration support
    - Complete set of Python API
    - Jupyter notebook integration

Supported compilers
===================

- Microsoft Visual Studio 2017 or newer
- GCC 7 or newer

Short history
===================

We started the development of Lightmetrica in 2014. The initial version was rather an experimental prototype. In 2015, we extended this prototype to `Lightmetrica Version 2 <https://github.com/hi2p-perim/lightmetrica-v2>`_. Fortunately, the development of Version 2 could be affiliated by `MITOU Program <https://www.ipa.go.jp/english/humandev/third.html>`_, a goverment-funded program by Ministry of Economy, Trade and Industry, Japan. The basic design of the renderer is fixed in this version. Successfully, we could use Version 2 in various research projects including several SIGGRAPH/SIGGRAPH Asia paper projects. In 2019, we decided to reimplement the whole renderer and we call this version as Version 3.

About Version 3
===================

Given the experiences and the accumulation of knowledge through various research projects, we have identified possible improvements and missing requirements or over-/under-specification of the design of Version 2. Some of them are identified and patched in the course of the projects on the top of the existing design, with suboptimal design choices that sacrifice the usability and performance of the framework. These codes are now becoming a technical debt through an accumulation of repetitive patches from each of research projects. Furthermore, some of the requirements are completely incompatible to the existing design of Version 2. In order to conduct a refactoring, at best, we could expect massive change to the existing design and implementation. 

Therefore, we decided to overhaul the framework based on a completely new codebase rather than refactoring. We can safely say although the renderer still preserves the name and continuing version number, the internal design is completely different. Some of the design are inherited from Version 2 and some are not.


Citation of this project
========================

You can use the following BibTex entry:

.. code-block:: bash

    @misc{lightmetrica-v3,
        author = {Hisanari Otsu},
        title = {Lightmetrica -- Research-oriented renderer (Version 3)},
        note = {http://lightmetrica.org},
        year = {2019},
    }
