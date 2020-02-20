.. _path_sampling:

Path sampling
######################

In this section we will discuss about path sampling API of Lightmetrica.
Implementing a rendering technique means you want to implement path sampling technique
and evaluating the contribution and probability that the path is being sampled.
Lightmetrica provides an API for path sampling and evaluation which supports typical use-cases to implement various rendering techniques. 
The purpose of this section is to give a correspondence between the formulation of light transport and the implementation in Lightmetrica. 

.. note::

    The math notations in this page is based on a seminal PhD thesis by Veach [1998], although there are slight differences.



Notations
================================

.. list-table::
    :widths: 20 40 40
    :header-rows: 1

    * - Symbol
      - Description
      - Type (depending on context)
    * - :math:`\mathbf{x}`
      - Position in a scene
      - ``SceneInteraction``, ``PointGeometry``, ``Vec3``
    * - :math:`\omega`
      - Direction
      - ``Vec3``
    * - :math:`dA`
      - Area measure
      -
    * - :math:`d\sigma`
      - Solid angle measure
      -
    * - :math:`d\sigma^\bot`
      - Projected solid angle measure
      -
    * - :math:`\mathcal{M}`
      - Set of points on scene surfaces
      -
    * - :math:`\mathcal{V}`
      - Set of points in volume
      -


Scene interaction point
================================

:cpp:class:`lm::SceneInteraction` structure represents a scene interaction associated to a point :math:`\mathbf{x}` in the scene. In the code we often name a variable of this type as ``sp`` (**s**\ cene interaction **p**\ oint).
The structure contains a type of the interaction ``sp.type``, the geometry information about the point ``sp.geom``, and the index of the scene node ``sp.primitive``.

The primitive index is mainly used internally to query the information of the scene from :cpp:class:`lm::Scene` class. Many of the sampling and evaluation functions under ``lm::path`` namespace use this index.

Scene interaction type
-------------------------------------

A type of the interaction ``sp.type`` is either of the following:

.. list-table::
    :widths: 20 40 40
    :header-rows: 1

    * - Notation
      - Description
      - Code
    * - :math:`\mathbf{x} \in \mathcal{M}_E`
      - Scene interaction is *camera endpoint*
      - ``sp.is_type == CameraEndpoint``
    * - :math:`\mathbf{x} \in \mathcal{M}_L`
      - Scene interaction is *light endpoint*
      - ``sp.type == LightEndpoint``
    * - :math:`\mathbf{x} \in \mathcal{M}`
      - Scene interaction is *surface interaction*
      - ``sp.type == SurfaceInteraction``
    * - :math:`\mathbf{x} \in \mathcal{V}`
      - Scene interaction is *medium interaction*
      - ``sp.type == MediumInteraction``

Also, some aggregate types are defined mainly for convenience:

.. list-table::
    :widths: 20 40 40
    :header-rows: 1

    * - Notation
      - Description
      - Code
    * - :math:`\mathbf{x} \in \mathcal{M}_L \cup \mathcal{M}_E`
      - Scene interaction is *endpoint*
      - ``(sp.type & Endpoint) > 0``
    * - :math:`\mathbf{x} \in \mathcal{M} \cup \mathcal{V}`
      - Scene interaction is *midpoint*
      - ``(sp.type & Midpoint) > 0``

Fixing interaction type
-------------------------------------

:cpp:func:`lm::as_type` function casts the scene interaction as a different type.
For instance, this function is useful when you want to enforce the scene interaction as an endpoint type.



Point geometry
================================

:cpp:class:`lm::PointGeometry` structure represents the geometry information associated to a point in a scene :math:`\mathbf{x}`.
In the code we often name a variable of this type as ``geom``.
The following table shows the correspondence between the math notations and members of the structure.

.. list-table::
    :widths: 20 40 40
    :header-rows: 1

    * - Notation
      - Description
      - Code
    * - :math:`\mathbf{n}_s(\mathbf{x})`
      - Shading normal at :math:`\mathbf{x}`
      - ``geom.n``
    * - :math:`\mathbf{n}_g(\mathbf{x})`
      - Geometric normal at :math:`\mathbf{x}`
      - ``geom.gn``
    * - :math:`(\mathbf{u},\mathbf{v})`
      - Orthonormal tangent vectors at :math:`\mathbf{x}`
      - ``geom.u``, ``geom.v``
    * - :math:`M_{\mathrm{world}}`
      - :math:`M_{\mathrm{world}} := [\mathbf{u}\; \mathbf{v}\; \mathbf{n}_s(\mathbf{x})]`
      - ``geom.to_world``
    * - :math:`M_{\mathrm{local}}`
      - :math:`M_{\mathrm{local}} := M_{\mathrm{world}}^T`
      - ``geom.to_local``

.. note::

  Depending on the context, the notations might be omitted. For instance, :math:`\mathbf{n}\equiv\mathbf{x}_s(\mathbf{x})` if by context the use of the shading normal for a point :math:`\mathbf{x}` is apparent.

Point geometry type
-------------------------------------

A point geometry has special flags representing specific configuration of the point:

- ``geom.degenerated`` representing the geometry around a point :math:`\mathbf{x}` is *degenerated*, namely, not associated to a surface. In this case, the normal and tangent vectors are undefined. 
- ``geom.infinite`` representing a virtual point far distant from a surface in certain direction. The point does not represent an actual point associated with a certain position in the scene. Also in this case, the normal and tangent vectors are undefined. Specifically in this case, ``geom.wo`` represents the direction toward the distant point.



Scene intersection query
================================

Scene intersection query is a basic building block of the rendering technique.
Our API supports two types of the scene intersection queries: *ray casting* and *visibility check*. 

Ray casting
-------------------------------------

- Function: :cpp:func:`lm::Scene::intersect`

*Ray casting* is an operation to find the closest next surface point :math:`\mathbf{x}_\mathcal{M}(\mathcal{x},\omega) \in \mathcal{M}` along with a direction :math:`\omega` from a point :math:`\mathbf{x}`, where

.. math::

    \mathbf{x}_\mathcal{M}(\mathcal{x},\omega) :=
      \mathbf{x} +
      \inf{\left\{ d>0 \mid \mathbf{x} + d\omega \in \mathcal{M} \right\} } \, \omega.

More specifically, the function can specify the range of distance :math:`[t_{\mathrm{min}},t_{\mathrm{max}}]` from the point :math:`\mathbf{x}`:

.. math::

    \mathbf{x}_\mathcal{M}(\mathcal{x},\omega, t_{\mathrm{min}},t_{\mathrm{max}}) :=
      \mathbf{x} +
      \inf{\left\{ d\in [ t_{\mathrm{min}},t_{\mathrm{max}} ] \mid \mathbf{x} + d\omega \in \mathcal{M} \right\} } \, \omega.

:cpp:func:`lm::Scene::intersect` function returns :cpp:class:`lm::SceneInteraction` of ``SurfaceInteraction`` type. The underlying ``geom`` contains the information about the intersected point.

.. note::

  The default values for the arguments ``tmin`` and ``tmax`` are :cpp:var:`lm::Eps` and :cpp:var:`Inf` respectively. ``tmin`` is set to :cpp:var:`Eps` to add tolerance to avoid self-intersection. 

Checking visibility
-------------------------------------

- Function: :cpp:func:`lm::Scene::visible`

The function evaluates the *visibility function* defined as

.. math::

  V(\mathbf{x}, \mathbf{y}) = 
  \begin{cases}
    1   &   \mathbf{x} \text{ and } \mathbf{y} \text{ are mutually visible,} \\
    0   &   \text{otherwise}.
  \end{cases}

Local ray/direction sampling
================================

A path construction comprises a combination of local sampling based on the point in a scene, which is important especially when you want to handle the path generation and evaluation implicitly, e.g., when you want to implement path tracing.

Primary ray sampling
-------------------------------------

- Sampling: :cpp:func:`lm::path::sample_primary_ray`
- PDF: :cpp:func:`lm::path::pdf_primary_ray`

The function samples a primary ray :math:`(\mathbf{x}, \omega)`.

.. math::

  (\mathbf{x}, \omega) \sim
  \begin{cases}
    p_{A\sigma^\bot L}(\cdot,\cdot)   & \text{if transport direction is } L\to E \\
    p_{A\sigma^\bot E}(\cdot,\cdot)   & \text{if transport direction is } E\to L.
  \end{cases}

If :math:`\mathbf{x}` and :math:`\omega` are independent,
the function is equivalent to evaluating :cpp:func:`lm::path::sample_position` and :cpp:func:`lm::path::sample_direction` separately.

The following table shows where each operation is implemented.

.. list-table::
    :header-rows: 1

    * - Operation
      - Implemented in
    * - :math:`(\mathbf{x}, \omega) \sim p_{A\sigma^\bot L}(\cdot,\cdot)`
      - :cpp:func:`lm::Light::sample_ray`
    * - :math:`p_{A\sigma^\bot L}(\mathbf{x}, \omega)`
      - :cpp:func:`lm::Light::pdf_ray`
    * - :math:`(\mathbf{x}, \omega) \sim p_{A\sigma^\bot E}(\cdot,\cdot)`
      - :cpp:func:`lm::Camera::sample_ray`
    * - :math:`p_{A\sigma^\bot E}(\mathbf{x}, \omega)`
      - :cpp:func:`lm::Camera::pdf_ray`

Endpoint sampling
-------------------------------------

- Sampling: :cpp:func:`lm::path::sample_position`
- PDF: :cpp:func:`lm::path::pdf_position`

The function samples an endpoint :math:`\mathbf{x}`.

.. math::

  \mathbf{x} \sim
  \begin{cases}
    p_{AL}(\cdot)   & \text{if transport direction is } L\to E \\
    p_{AE}(\cdot)   & \text{if transport direction is } E\to L.
  \end{cases}

The following table shows where each operation is implemented.

.. list-table::
    :header-rows: 1

    * - Operation
      - Implemented in
    * - :math:`\mathbf{x} \sim p_{AL}(\cdot)`
      - :cpp:func:`lm::Light::sample_position`
    * - :math:`p_{AL}(\mathbf{x})`
      - :cpp:func:`lm::Light::pdf_position`
    * - :math:`\mathbf{x} \sim p_{AE}(\cdot)`
      - :cpp:func:`lm::Camera::sample_position`
    * - :math:`p_{AE}(\mathbf{x})`
      - :cpp:func:`lm::Camera::pdf_position`

Direction sampling
-------------------------------------

- Sampling: :cpp:func:`lm::path::sample_direction`
- PDF: :cpp:func:`lm::path::pdf_direction`

The function samples a direction :math:`\omega` originated from a current position :math:`\mathbf{x}`:

.. math::

  \omega \sim
  \begin{cases}
    p_{\sigma^\bot L}(\cdot\mid\mathbf{x})    &   \text{if } \mathbf{x} \in \mathcal{M}_L \\
    p_{\sigma^\bot E}(\cdot\mid\mathbf{x})    &   \text{if } \mathbf{x} \in \mathcal{M}_E \\
    p_{\sigma^\bot \mathrm{bsdf}}(\cdot\mid\mathbf{x})  &   \text{if } \mathbf{x} \in \mathcal{M} \\
    p_{\sigma^\bot \mathrm{phase}}(\cdot\mid\mathbf{x}) &   \text{if } \mathbf{x} \in \mathcal{V}.
  \end{cases}

The following table shows where each operation is implemented.

.. list-table::
    :header-rows: 1

    * - Operation
      - Implemented in
    * - :math:`\omega \sim p_{\sigma^\bot L}(\cdot\mid\mathbf{x})`
      - :cpp:func:`lm::Light::sample_direction`
    * - :math:`p_{\sigma^\bot L}(\omega\mid\mathbf{x})`
      - :cpp:func:`lm::Light::pdf_direction`
    * - :math:`\omega \sim p_{\sigma^\bot E}(\cdot\mid\mathbf{x})`
      - :cpp:func:`lm::Camera::sample_direction`
    * - :math:`p_{\sigma^\bot E}(\omega\mid\mathbf{x})`
      - :cpp:func:`lm::Camera::pdf_direction`
    * - :math:`\omega \sim p_{\sigma^\bot \mathrm{bsdf}}(\cdot\mid\mathbf{x})`
      - :cpp:func:`lm::Material::sample_direction`
    * - :math:`p_{\sigma^\bot \mathrm{bsdf}}(\omega\mid\mathbf{x})`
      - :cpp:func:`lm::Material::pdf_direction`
    * - :math:`\omega \sim p_{\sigma^\bot \mathrm{phase}}(\cdot\mid\mathbf{x})`
      - :cpp:func:`lm::Phase::sample_direction`
    * - :math:`p_{\sigma^\bot \mathrm{phase}}(\omega\mid\mathbf{x})`
      - :cpp:func:`lm::Phase::pdf_direction`

Direct endpoint sampling
-------------------------------------

- Sampling: :cpp:func:`lm::path::sample_direct`
- PDF: :cpp:func:`lm::path::pdf_direct`

The function samples a direction :math:`\omega` directly toward an endpoint based on the current position :math:`\mathbf{x}`. This sampling strategy is mainly used to implement next event estimation.

.. math::

  \omega \sim
  \begin{cases}
    p_{\sigma^\bot \mathrm{directL}}(\cdot\mid\mathbf{x})    & \text{if transport direction is } E\to L \\
    p_{\sigma^\bot \mathrm{directE}}(\cdot\mid\mathbf{x})    & \text{if transport direction is } L\to E.
  \end{cases}

The following table shows where each operation is implemented.

.. list-table::
    :header-rows: 1

    * - Operation
      - Implemented in
    * - :math:`\omega \sim p_{\sigma^\bot \mathrm{directL}}(\cdot\mid\mathbf{x})`
      - :cpp:func:`lm::Light::sample_direct`
    * - :math:`p_{\sigma^\bot \mathrm{directL}}(\omega\mid\mathbf{x})`
      - :cpp:func:`lm::Light::pdf_direct`
    * - :math:`\omega \sim p_{\sigma^\bot \mathrm{directE}}(\cdot\mid\mathbf{x})`
      - :cpp:func:`lm::Camera::sample_direct`
    * - :math:`p_{\sigma^\bot \mathrm{directE}}(\omega\mid\mathbf{x})`
      - :cpp:func:`lm::Camera::pdf_direct`

Evaluating directional components
-------------------------------------

- Function: :cpp:func:`lm::path::eval_contrb_direction`

The function evaluates directional component of path integral :math:`f(\mathbf{x}, \omega_i,\omega_o)`, where

.. math::

  f(\mathbf{x},\omega_i,\omega_o) :=
  \begin{cases}
    L_e(\mathbf{x}, \omega_o)         & \mathbf{x}\in\mathcal{M}_L \\
    W_e(\mathbf{x}, \omega_o)         & \mathbf{x}\in\mathcal{M}_E \\
    f_s(\mathbf{x},\omega_i,\omega_o) & \mathbf{x}\in\mathcal{M} \\
    \mu_s(\mathbf{x}) f_p(\mathbf{x},\omega_i,\omega_o) & \mathbf{x}\in\mathcal{V}.
  \end{cases}

The following table shows where each operation is implemented.

.. list-table::
    :header-rows: 1

    * - Operation
      - Implemented in
    * - :math:`L_e(\mathbf{x}, \omega_o)`
      - :cpp:func:`lm::Light::eval`
    * - :math:`W_e(\mathbf{x}, \omega_o)`
      - :cpp:func:`lm::Camera::eval`
    * - :math:`f_s(\mathbf{x},\omega_i,\omega_o)`
      - :cpp:func:`lm::Material::eval`
    * - :math:`\mu_s(\mathbf{x})`
      - N/A
    * - :math:`f_p(\mathbf{x},\omega_i,\omega_o)`
      - :cpp:func:`lm::Phase::eval`

.. note::

  :math:`\omega_i` is not used when :math:`\mathbf{x}` is endpoint.
  Also, :math:`\omega_o` always represents outgoing direction irrespective to the transport directions,
  that is, the same direction as the transport direction.

.. note::

  :cpp:func:`lm::path::eval_contrb_direction` takes ``trans_dir`` as an argument, which is used to handle non-symmetric scattering described in Chapter 5 of Veach's thesis.

Computing Jacobian
-------------------------------------

We provide some functions to compute Jacobians (Jacobian determinants) to convert density functions according to a different measure.

Bidirectional path sampling
===========================

Some rendering techniques such as bidirectional path tracing are based on *bidirectional path sampling*, which explicitly manages a structure of a light transport path in the process of sampling and evaluation.
In this section, we will introduce the related API for bidirectional path sampling.

.. note::

  Currently bidirectional path sampling of the framework only supports light transport on surfaces,
  although we have a plan to support volumetric light transport.

Notations
-------------------------------------

.. list-table::
    :widths: 20 40 40
    :header-rows: 1

    * - Symbol
      - Description
      - Type (depending on context)
    * - :math:`\mathbf{x}`
      - Path vertex
      - ``Vert``
    * - :math:`\bar{x}`
      - Light transport path (or just path)
      - ``Path``
    * - :math:`\bar{x}_L`
      - Light subpath
      - ``Path``
    * - :math:`\bar{x}_E`
      - Eye subpath
      - ``Path``
    * - :math:`(s,t)`
      - Strategy index of bidirectional path sampling
      - ``(int, int)``
    * - :math:`s`
      - Number of vertices in light subpath
      - ``int``
    * - :math:`t`
      - Number of vertices in eye subpath
      - ``int``

Light transport path
-------------------------------------

A *path* :math:`\bar{x}` is defined by a sequence of path vertices.
We denote the path with the number of vertices :math:`k` as :math:`\bar{x}_k:=\mathbf{x}_0\mathbf{x}_2\dots\mathbf{x}_{k-1}`.
We often omit the subscript :math:`k` depending on the context.

- A path is *full path* if the path constitutes of a complete light transport path, where :math:`\mathbf{x}_0\in\mathcal{M}_L`, :math:`\mathbf{x}_{k-1}\in\mathcal{M}_E`. In the context without ambiguity, we call a *full path* as merely a *path*.
- A path is *subpath* if the path starts but not ends its vertices on the endpoints. Note that the subpath always starts from an endpoint, irrespective to the type of the endpoint. If :math:`\mathbf{x}_0\in\mathcal{M}_L`, the subpath is called *light subpath*. If :math:`\mathbf{x}_0\in\mathcal{M}_E`, the subpath is called *eye subpath*.

In the framework, :cpp:class:`lm::Path` structure represents a path, which holds a vector ``.vs``  of :cpp:class:`lm::Vert` representing a path vertex.
A path vertex structure is a tuple of surface interaction ``.sp`` and ``.specular`` flag representing whether the vertex type is specular. 

.. note::

  The order of the vector ``.vs`` depends on the type of the path. If a path represents a full path, the vector always starts from a vertex representing light endpoint and ends with camera endpoint. On the other hand, if a path represents a subpath, the vector starts from an endpoint irrespective to the type of endpoint.

The correspondence between notations and the operations over the path structure is summarized in the following table. 

.. list-table::
    :widths: 20 40 40
    :header-rows: 1

    * - Notation
      - Description
      - Code
    * - :math:`k`
      - Number of path vertices
      - :cpp:func:`lm::Path::num_verts`
    * - :math:`k+1`
      - Path length
      - :cpp:func:`lm::Path::num_edges`
    * - :math:`\mathbf{x}_i`
      - :math:`i`-th path vertex from :math:`\mathbf{x}_0`
      - :cpp:func:`lm::Path::vertex_at` with ``trans_dir=LE``
    * - :math:`\mathbf{x}_{k-1-i}`
      - :math:`i`-th path vertex from :math:`\mathbf{x}_{k-1}`
      - :cpp:func:`lm::Path::vertex_at` with ``trans_dir=EL``
    * - :math:`\mathbf{x}_i`
      - :math:`i`-th path vertex from :math:`\mathbf{x}_0`
      - :cpp:func:`lm::Path::subpath_vertex_at`

.. note::

  :cpp:func:`lm::Path::vertex_at` or :cpp:func:`lm::Path::subpath_vertex_at` returns
  a pointer to a path vertex and ``nullptr`` if the index is out of bound,
  which is intentional to simplify the implementation.


Sampling subpath
-------------------------------------

- Function: :cpp:func:`lm::path::sample_subpath`

The function samples a subpath up to the given maximum number of vertices ``max_verts``. The type of subpath can be configured by the argument ``trans_dir``.


 

Bidirectional path sampling
-------------------------------------


Computing raster position
-------------------------------------







Evaluating path contribution
-------------------------------------

Evaluating bidirectional path PDF
-------------------------------------

Evaluating MIS weight
-------------------------------------

Evaluating sampling weight
-------------------------------------

Subpath contribution
-------------------------------------

Connection term
-------------------------------------

Unweighted contribution
-------------------------------------

