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

.. note::

    This section would be better understood in conjunction with an actual implementation of a renderer.
    You can find various built-in renderer implementations in ``src/renderer`` directory.





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
    * - :math:`\omega_{\mathbf{x} \to \mathbf{y}}`
      - Direction from :math:`\mathbf{x}` to :math:`\mathbf{y}`
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
    * - :math:`\mathcal{S}^2`
      - Unit sphere
      -


.. _path_sampling_scene_interaction_point:

Scene interaction point
================================

:cpp:class:`lm::SceneInteraction` structure represents a scene interaction associated to a point :math:`\mathbf{x}` in the scene. In the code we often name a variable of this type as ``sp`` (**s**\ cene interaction **p**\ oint).
The structure contains a type of the interaction ``sp.type``, the geometry information about the point ``sp.geom``, and the index of the scene node ``sp.primitive``.

The primitive index is mainly used internally to query the information of the scene from :cpp:class:`lm::Scene` class. Many of the sampling and evaluation functions under ``lm::path`` namespace use this index.

.. _path_sampling_scene_interaction_type:

Scene interaction type
-------------------------------------

A type of the interaction ``sp.type`` is either of the following:

.. list-table::
    :widths: 20 40 40
    :header-rows: 1

    * - Notation
      - Description
      - Code
    * - :math:`\mathbf{x} \in \mathcal{M}_E \subseteq \mathcal{M}`
      - Scene interaction is *camera endpoint*
      - ``sp.is_type(CameraEndpoint)``
    * - :math:`\mathbf{x} \in \mathcal{M}_L \subseteq \mathcal{M}`
      - Scene interaction is *light endpoint*
      - ``sp.is_type(LightEndpoint)``
    * - :math:`\mathbf{x} \in \mathcal{M}_S \subseteq \mathcal{M}`
      - Scene interaction is *surface interaction*
      - ``sp.is_type(SurfaceInteraction)``
    * - :math:`\mathbf{x} \in \mathcal{V}`
      - Scene interaction is *medium interaction*
      - ``sp.is_type(MediumInteraction)``

Also, some aggregate types are defined mainly for convenience:

.. list-table::
    :widths: 20 40 40
    :header-rows: 1

    * - Notation
      - Description
      - Code
    * - :math:`\mathbf{x} \in \mathcal{M}_L \cup \mathcal{M}_E`
      - Scene interaction is *endpoint*
      - ``sp.is_type(Endpoint)``
    * - :math:`\mathbf{x} \in \mathcal{M}_S \cup \mathcal{V}`
      - Scene interaction is *midpoint*
      - ``sp.is_type(Midpoint)``

Fixing interaction type
-------------------------------------

:cpp:func:`lm::as_type` function casts the scene interaction as a different type.
For instance, this function is useful when you want to enforce the scene interaction as an endpoint type.




.. _path_sampling_point_geometry:

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

A point geometry has special flags representing specific configuration of the point.

.. _path_sampling_degenerated_point:

Degenerated point
""""""""""""""""""""""""""""

``geom.degenerated`` representing the geometry around a point :math:`\mathbf{x}` is *degenerated*, namely, the surface around the point is collapsed to a point or a line. In this case, the normal and tangent vectors are undefined. Notation: :math:`\mathbf{x}\in\mathcal{S}_{\mathrm{deg}}`.

.. note::
  Points in a volume is always degenerated: :math:`\mathcal{V}\subseteq\mathcal{S}_{\mathrm{deg}}`.

.. _path_sampling_infinitely_distant_point:

Infinitely distant point
""""""""""""""""""""""""""""

``geom.infinite`` representing a virtual point far distant from a surface in certain direction :math:`\omega`. The point does not represent an actual point associated with a certain position in the scene. Also in this case, the normal and tangent vectors are undefined. Specifically in this case, ``geom.wo`` represents the direction toward the distant point. Notation: :math:`\mathbf{x}\in\mathcal{S}_{\mathrm{inf}}`. Since the point is characterized by a direction :math:`\omega` we sometime denote the point as :math:`\mathbf{x}(\omega)\in\mathcal{S}_{\mathrm{inf}}`.


Component index
================================

A scene interaction can be associated with a *component* information,
which is used to differentiate the behavior of the sampling and evaluation related to the interaction.
A component is denoted by a *component index*, an integer value that specify the index of the component of the interaction.
For instance, this feature can be used to implement sampling and evaluation of multiple component materials.
Furthermore, the handling of perfect specular materials can be implemented using this feature.
In the code, we denote the component index as ``comp``.
Later we will discuss about the detail of the usage in the API.

.. note::

  The component index is not included in the information accessible as :cpp:class:`lm::SceneInteraction` structure, since we want to handle the scene information without components being selected, e.g., the intersected surface point via ray casting.


Ray-scene intersection query
================================

Ray-scene intersection query is a basic building block of the rendering technique.
Our API supports two types of the ray-scene intersection queries: *ray casting* and *visibility check*. 

Ray casting
-------------------------------------

The query is implemented in :cpp:func:`lm::Scene::intersect` function.
*Ray casting* is an operation to find the closest next surface point :math:`\mathbf{x}_\mathcal{M}(\mathcal{x},\omega) \in \mathcal{M}_S` along with a direction :math:`\omega` from a point :math:`\mathbf{x}` within the range of the distance :math:`[t_{\mathrm{min}},t_{\mathrm{max}}]`, where

.. math::

    \mathbf{x}_\mathcal{M}(\mathbf{x},\omega, t_{\mathrm{min}},t_{\mathrm{max}})
      &= \mathbf{x} + d_\mathcal{M}(\mathbf{x},\omega,t_{\mathrm{min}},t_{\mathrm{max}}) \, \omega, \\
    d_\mathcal{M}(\mathbf{x},\omega,t_{\mathrm{min}},t_{\mathrm{max}})
      &= \inf{\left\{ d \in [t_{\mathrm{min}},t_{\mathrm{max}}] \mid \mathbf{x} + d\omega \in \mathcal{M}_S \right\} }.

:cpp:func:`lm::Scene::intersect` function returns :cpp:class:`lm::SceneInteraction` of ``SurfaceInteraction`` type. The underlying ``geom`` contains the information about the intersected point.
Note that :math:`\mathbf{x}_\mathcal{M}(\mathbf{x},\omega, t_{\mathrm{min}},\infty) \in \mathcal{S}_{\mathrm{inf}}` if :math:`d_\mathcal{M}(\mathbf{x},\omega,t_{\mathrm{min}},\infty) = \infty`.

.. note::

  The default values for the arguments ``tmin`` and ``tmax`` are :cpp:var:`lm::Eps` and :cpp:var:`Inf` respectively. ``tmin`` is set to :cpp:var:`Eps` to add tolerance to avoid self-intersection. 

Checking visibility
-------------------------------------

The query is implemented in :cpp:func:`lm::Scene::visible` function.
The function evaluates the *visibility function* defined as

.. math::

  V(\mathbf{x}, \mathbf{y}) = 
  \begin{cases}
    1   &   \mathbf{x} \text{ and } \mathbf{y} \text{ are mutually visible,} \\
    0   &   \text{otherwise}.
  \end{cases}

More precisely, we use the extended version of the function incorporating the properties of point geometry information. The extended visiblity function is defined for :math:`(\mathbf{x}, \mathbf{y})` where :math:`\mathbf{x}` and :math:`\mathbf{x}` are not both in :math:`\mathcal{S}_{\mathrm{inf}}` as 

.. math::

  V(\mathbf{x}, \mathbf{y}) &= 
  \begin{cases}
    V_1(\mathbf{x}, \mathbf{y})   &   \mathbf{x}\in\mathcal{S}_{\mathrm{inf}} \lor \mathbf{y}\in\mathcal{S}_{\mathrm{inf}} \\
    V_2(\mathbf{x}, \mathbf{y})   &   \mathbf{x}\notin\mathcal{S}_{\mathrm{inf}} \land \mathbf{y}\notin\mathcal{S}_{\mathrm{inf}},
  \end{cases} \\
  V_1(\mathbf{x}, \mathbf{y}) &=
  \begin{cases}
    1   &     \mathbf{y}\in\mathcal{S}_{\mathrm{inf}} \land d_\mathcal{M}(\mathbf{x},\omega_{\mathbf{x}\to\mathbf{y}}, 0, \infty) = \infty \text{ or} \\
        &     \mathbf{x}\in\mathcal{S}_{\mathrm{inf}} \land d_\mathcal{M}(\mathbf{y},\omega_{\mathbf{y}\to\mathbf{x}}, 0, \infty) = \infty, \\
    0   &     \text{otherwise},
  \end{cases} \\
  V_2(\mathbf{x}, \mathbf{y}) &=
  \begin{cases}
    1   &   d_\mathcal{M}(\mathbf{x},\omega, 0, \| \mathbf{x}-\mathbf{y} \| ) = \| \mathbf{x}-\mathbf{y} \|, \\
    0   &   \text{otherwise}.
  \end{cases}



Local ray/direction sampling
================================

A path construction comprises a combination of local sampling based on the point in a scene, which is important especially when you want to handle the path generation and evaluation implicitly, e.g., when you want to implement path tracing.

Aggregated solid angle measure for direction sampling
-------------------------------------------------------

The framework defines various direction sampling techniques, many of which take samples from conditional distribution given a point :math:`\mathbf{x}`. The density function of the conditional distribution is either defined over solid angle measure or projected solid angle measure according to the type of the point geometry. 

To generalize the two cases and simplify the interface, we define *aggregated solid angle measure* :math:`d\sigma^*` which alternates the measure by the degeneracy of the point geometry:

.. math::

  d\sigma^*(\omega\mid\mathbf{x}) =
  \begin{cases}
    d\sigma(\omega\mid\mathbf{x})       & \mathbf{x}\in\mathcal{S}_{\mathrm{deg}}, \\
    d\sigma^\bot(\omega\mid\mathbf{x})  & \mathbf{x}\notin\mathcal{S}_{\mathrm{deg}},
  \end{cases}

Using this measure, we can define the aggregated PDF:

.. math::

  p_{\sigma^*}(\omega\mid\mathbf{x}) = 
  \begin{cases}
    p_{\sigma}(\omega\mid\mathbf{x})         &   \mathbf{x}\in\mathcal{S}_{\mathrm{deg}}, \\
    p_{\sigma^\bot}(\omega\mid\mathbf{x})    &   \mathbf{x}\notin\mathcal{S}_{\mathrm{deg}}.
  \end{cases}

Aggregated throughput measure for primary ray sampling
-------------------------------------------------------

The primary ray is sampled from the joint distribution. 
Similarly to the previous case, we want to define the generalized measure to support various use-cases of the primary ray sampling. We categorize the measure by two types according to the type of the endpoint being sampled from the joint distribution.

.. math::

  d\mu^*(\mathbf{x},\omega) &=
  \begin{cases}
    d\sigma(\omega) dA^\bot(\mathbf{x}\mid\omega)   & \mathbf{x}\in\mathcal{S}_{\mathrm{inf}} \\
    dA(\mathbf{x}) d\sigma^*(\omega\mid\mathbf{x})    & \mathbf{x}\notin\mathcal{S}_{\mathrm{inf}}
  \end{cases} \\
  &=
  \begin{cases}
    d\sigma(\omega) dA^\bot(\mathbf{x}\mid\omega)     & \mathbf{x}\in\mathcal{S}_{\mathrm{inf}} \\
    dA(\mathbf{x}) d\sigma(\omega\mid\mathbf{x})      & \mathbf{x}\notin\mathcal{S}_{\mathrm{inf}} \land \mathbf{x}\in\mathcal{S}_{\mathrm{deg}} \\
    dA(\mathbf{x}) d\sigma^\bot(\omega\mid\mathbf{x}) & \mathbf{x}\notin\mathcal{S}_{\mathrm{inf}} \land \mathbf{x}\notin\mathcal{S}_{\mathrm{deg}},
  \end{cases}
  
where :math:`\mu^*` is a *throughput measure* [Veach 1998, Chapter 4.1] defined for the space of rays :math:`\mathcal{M}\times\mathcal{S}^2`. The second line expands the definition of the aggregated solid angle measure, which enable to support the cases where the ray originated from the degenerated point (e.g., pinhole camera).

Note that in the case of :math:`\mathbf{x}\in\mathcal{S}_{\mathrm{inf}}`, the position sampling happens on the virtual plane perpendicular to the ray direction, which is reflected by the projected area measure :math:`dA^\bot`.

Similarly, the joint PDF can be defined as

.. math::

  p_{\mu^*}(\mathbf{x},\omega) &=
  \begin{cases}
    p_{\sigma}(\omega) p_{A^\bot}(\mathbf{x}\mid\omega)     & \mathbf{x}\in\mathcal{S}_{\mathrm{inf}} \\
    p_A(\mathbf{x}) p_\sigma(\omega\mid\mathbf{x})      & \mathbf{x}\notin\mathcal{S}_{\mathrm{inf}} \land \mathbf{x}\in\mathcal{S}_{\mathrm{deg}} \\
    p_A(\mathbf{x}) p_{\sigma^\bot}(\omega\mid\mathbf{x}) & \mathbf{x}\notin\mathcal{S}_{\mathrm{inf}} \land \mathbf{x}\notin\mathcal{S}_{\mathrm{deg}}.
  \end{cases}

.. _path_sampling_component_sampling:

Component sampling
-------------------------------------

- Sampling: :cpp:func:`lm::path::sample_component`
- PDF: :cpp:func:`lm::path::pdf_component`

The function samples a component index :math:`j`.

.. math::

  j \sim
  p_c(\cdot) =
  \begin{cases}
    p_{c,L}(\cdot\mid\mathbf{x})     & \text{if } \mathbf{x} \in \mathcal{M}_L \\
    p_{c,E}(\cdot\mid\mathbf{x})     & \text{if } \mathbf{x} \in \mathcal{M}_E \\
    p_{c,\mathrm{bsdf}}(\cdot\mid\mathbf{x})     & \text{if } \mathbf{x} \in \mathcal{M}_S \\
    p_{c,\mathrm{phase}}(\cdot\mid\mathbf{x})     & \text{if } \mathbf{x} \in \mathcal{V}.
  \end{cases}

Currently component sampling is only supported for the surface interactions.
In other cases the component index is fixed to 0 (with probability 1).
The following table shows where the operation is implemented.

.. list-table::
    :header-rows: 1

    * - Operation
      - Implemented in
    * - :math:`j \sim p_{c,\mathrm{bsdf}}(\cdot\mid\mathbf{x})`
      - :cpp:func:`lm::Material::sample_component`
    * - :math:`p_{c,\mathrm{bsdf}}(j\mid\mathbf{x})`
      - :cpp:func:`lm::Material::pdf_component`

.. _path_sampling_primary_ray_sampling:

Primary ray sampling
-------------------------------------

- Sampling: :cpp:func:`lm::path::sample_primary_ray`
- PDF: :cpp:func:`lm::path::pdf_primary_ray`

The function samples a primary ray :math:`(\mathbf{x}, \omega)`.

.. math::

  (\mathbf{x}, \omega) \sim
  \begin{cases}
    p_{\mu^* L}(\cdot,\cdot)   & \text{if transport direction is } L\to E \\
    p_{\mu^* E}(\cdot,\cdot)   & \text{if transport direction is } E\to L.
  \end{cases}

If :math:`\mathbf{x}` and :math:`\omega` are independent,
the function is equivalent to evaluating :cpp:func:`lm::path::sample_position` and :cpp:func:`lm::path::sample_direction` separately.

The following table shows where each operation is implemented.

.. list-table::
    :header-rows: 1

    * - Operation
      - Implemented in
    * - :math:`(\mathbf{x}, \omega) \sim p_{\mu^* L}(\cdot,\cdot)`
      - :cpp:func:`lm::Light::sample_ray`
    * - :math:`p_{\mu^* L}(\mathbf{x}, \omega)`
      - :cpp:func:`lm::Light::pdf_ray`
    * - :math:`(\mathbf{x}, \omega) \sim p_{\mu^* E}(\cdot,\cdot)`
      - :cpp:func:`lm::Camera::sample_ray`
    * - :math:`p_{\mu^* E}(\mathbf{x}, \omega)`
      - :cpp:func:`lm::Camera::pdf_ray`

.. _path_sampling_endpoint_sampling:

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

.. _path_sapmling_direction_sampling:

Direction sampling
-------------------------------------

- Sampling: :cpp:func:`lm::path::sample_direction`
- PDF: :cpp:func:`lm::path::pdf_direction`

The function samples a direction :math:`\omega` originated from a current position :math:`\mathbf{x}` with the component index :math:`j`:

.. math::

  \omega \sim
  p_{\sigma^* \to}(\cdot\mid\mathbf{x},j) =
  \begin{cases}
    p_{\sigma^* L}(\cdot\mid\mathbf{x})    &   \text{if } \mathbf{x} \in \mathcal{M}_L \\
    p_{\sigma^* E}(\cdot\mid\mathbf{x})    &   \text{if } \mathbf{x} \in \mathcal{M}_E \\
    p_{\sigma^* \mathrm{bsdf}}(\cdot\mid\mathbf{x},j)  &   \text{if } \mathbf{x} \in \mathcal{M}_S \\
    p_{\sigma^* \mathrm{phase}}(\cdot\mid\mathbf{x}) &   \text{if } \mathbf{x} \in \mathcal{V}.
  \end{cases}

The following table shows where each operation is implemented.

.. list-table::
    :header-rows: 1

    * - Operation
      - Implemented in
    * - :math:`\omega \sim p_{\sigma^* L}(\cdot\mid\mathbf{x})`
      - :cpp:func:`lm::Light::sample_direction`
    * - :math:`p_{\sigma^* L}(\omega\mid\mathbf{x})`
      - :cpp:func:`lm::Light::pdf_direction`
    * - :math:`\omega \sim p_{\sigma^* E}(\cdot\mid\mathbf{x})`
      - :cpp:func:`lm::Camera::sample_direction`
    * - :math:`p_{\sigma^* E}(\omega\mid\mathbf{x})`
      - :cpp:func:`lm::Camera::pdf_direction`
    * - :math:`\omega \sim p_{\sigma^* \mathrm{bsdf}}(\cdot\mid\mathbf{x},j)`
      - :cpp:func:`lm::Material::sample_direction`
    * - :math:`p_{\sigma^* \mathrm{bsdf}}(\omega\mid\mathbf{x},j)`
      - :cpp:func:`lm::Material::pdf_direction`
    * - :math:`\omega \sim p_{\sigma^* \mathrm{phase}}(\cdot\mid\mathbf{x})`
      - :cpp:func:`lm::Phase::sample_direction`
    * - :math:`p_{\sigma^* \mathrm{phase}}(\omega\mid\mathbf{x})`
      - :cpp:func:`lm::Phase::pdf_direction`

.. _path_sampling_direct_endpoint_sampling:

Direct endpoint sampling
-------------------------------------

- Sampling: :cpp:func:`lm::path::sample_direct`
- PDF: :cpp:func:`lm::path::pdf_direct`

The function samples a direction :math:`\omega` directly toward an endpoint based on the current position :math:`\mathbf{x}`. This sampling strategy is mainly used to implement next event estimation.

.. math::

  \omega \sim
  \begin{cases}
    p_{\sigma^* \mathrm{directL}}(\cdot\mid\mathbf{x})    & \text{if transport direction is } E\to L \\
    p_{\sigma^* \mathrm{directE}}(\cdot\mid\mathbf{x})    & \text{if transport direction is } L\to E.
  \end{cases}

The following table shows where each operation is implemented.

.. list-table::
    :header-rows: 1

    * - Operation
      - Implemented in
    * - :math:`\omega \sim p_{\sigma^* \mathrm{directL}}(\cdot\mid\mathbf{x})`
      - :cpp:func:`lm::Light::sample_direct` with ``trans_dir = LE``
    * - :math:`p_{\sigma^* \mathrm{directL}}(\omega\mid\mathbf{x})`
      - :cpp:func:`lm::Light::pdf_direct` with ``trans_dir = LE``
    * - :math:`\omega \sim p_{\sigma^* \mathrm{directE}}(\cdot\mid\mathbf{x})`
      - :cpp:func:`lm::Camera::sample_direct` with ``trans_dir = EL``
    * - :math:`p_{\sigma^* \mathrm{directE}}(\omega\mid\mathbf{x})`
      - :cpp:func:`lm::Camera::pdf_direct` with ``trans_dir = EL``

.. _path_evaluating_directional_components:

Evaluating directional components
-------------------------------------

- Function: :cpp:func:`lm::path::eval_contrb_direction`

The function evaluates directional component of path integral :math:`f_{s\Sigma}(\mathbf{x},j, \omega_i,\omega_o)`, where

.. math::

  f_{s\Sigma}(\mathbf{x},j,\omega_i,\omega_o) =
  \begin{cases}
    L_e(\mathbf{x}, \omega_o)         & \mathbf{x}\in\mathcal{M}_L \\
    W_e(\mathbf{x}, \omega_o)         & \mathbf{x}\in\mathcal{M}_E \\
    f_{\mathrm{bsdf}\Sigma}(\mathbf{x},j,\omega_i,\omega_o) & \mathbf{x}\in\mathcal{M}_S \\
    \mu_s(\mathbf{x}) f_{\mathrm{phase}}(\mathbf{x},\omega_i,\omega_o) & \mathbf{x}\in\mathcal{V},
  \end{cases}

where :math:`\Sigma\in\{ L,E \}`. :math:`\Sigma` corresponds to the transport direction, which is necessary to handle non-symmetric scattering described in Chapter 5 of Veach's thesis.
The following table shows where each operation is implemented.

.. list-table::
    :header-rows: 1

    * - Operation
      - Implemented in
    * - :math:`L_e(\mathbf{x},\omega_o)`
      - :cpp:func:`lm::Light::eval`
    * - :math:`W_e(\mathbf{x},\omega_o)`
      - :cpp:func:`lm::Camera::eval`
    * - :math:`f_{\mathrm{bsdf}L}(\mathbf{x},j,\omega_i,\omega_o)`
      - :cpp:func:`lm::Material::eval` with ``trans_dir = LE``
    * - :math:`f_{\mathrm{bsdf}E}(\mathbf{x},j,\omega_i,\omega_o)`
      - :cpp:func:`lm::Material::eval` with ``trans_dir = EL``
    * - :math:`\mu_s(\mathbf{x})`
      - N/A
    * - :math:`f_{\mathrm{phase}}(\mathbf{x},\omega_i,\omega_o)`
      - :cpp:func:`lm::Phase::eval`

.. note::

  :math:`\omega_i` is not used when :math:`\mathbf{x}` is endpoint.
  Also, :math:`\omega_o` always represents outgoing direction irrespective to the transport directions,
  that is, the same direction as the transport direction.

Transforming probability densities
======================================================

The framework provides the functions to transform density functions according to a different measure.

Solid angle to projected solid angle
-------------------------------------

:cpp:func:`lm::surface::convert_pdf_SA_to_projSA` implements the conversion:

.. math::

  p_{\sigma^\bot}(\omega\mid\mathbf{x})
    = \left| \frac{d\sigma}{d\sigma^\bot} \right| p_{\sigma}(\omega\mid\mathbf{x})
    = \frac{p_{\sigma}(\omega)}{\| \mathbf{n}(\mathbf{x}) \cdot \omega \|}.

.. _path_sampling_aggregated_solid_angle_to_area:

Aggregated solid angle to area
-------------------------------------

.. _path_sampling_extended_geometry_term:

To achieve the transformation of densities from aggregated solid angle to area measure according to the point geometry types transparently, we define the *extended geometry term* as

.. math::

  G(\mathbf{x}, \mathbf{y}) &=
    \frac{D(\mathbf{x}, \mathbf{y}) V(\mathbf{x}, \mathbf{y}) D(\mathbf{y}, \mathbf{x})}
         {d^2(\mathbf{x}, \mathbf{y})}, \\
  D(\mathbf{x}, \mathbf{y}) &=
    \begin{cases}
      \left| \mathbf{n}(\mathbf{x}) \cdot \omega_{\mathbf{x} \to \mathbf{y}} \right|
        & \mathbf{x}\notin\mathcal{S}_{\mathrm{deg}} \\
      1 & \mathbf{x}\in\mathcal{S}_{\mathrm{deg}},
    \end{cases} \\
  d^2(\mathbf{x}, \mathbf{y}) &=
    \begin{cases}
      \| \mathbf{x} - \mathbf{y} \|^2
        & \mathbf{x}\notin\mathcal{S}_{\mathrm{inf}} \land \mathbf{y}\notin\mathcal{S}_{\mathrm{inf}} \\
      1 & \text{otherwise}.
    \end{cases}

:cpp:func:`lm::surface::geometry_term` function evaluates the term, but assuming that :math:`\mathbf{x}` and :math:`\mathbf{y}` are mutually visible (thus :math:`V(\mathbf{x}, \mathbf{y})=1`).
:cpp:func:`lm::surface::convert_pdf_projSA_to_area` function internally uses this function to implement the conversion of the densities, which allows to convert the densities according to the point geometry type:

.. math::

  p_A(\mathbf{y}\mid\mathbf{x})
    = \underbrace{\left| \frac{d\sigma^*}{dA} \right|}_{G(\mathbf{x}, \mathbf{y})}
      p_{\sigma^*}(\omega\mid\mathbf{x})
    =
    \begin{cases}
      \left| \frac{d\sigma}{dA} \right|      p_{\sigma}(\omega\mid\mathbf{x})
        & \mathbf{x}\in\mathcal{S}_{\mathrm{deg}}, \\
      \left| \frac{d\sigma^\bot}{dA} \right| p_{\sigma^\bot}(\omega\mid\mathbf{x})
        & \mathbf{x}\notin\mathcal{S}_{\mathrm{deg}},
    \end{cases}

where :math:`\omega = \omega_{\mathbf{x}\to\mathbf{y}}`.
This is especially useful when the conversion function is used in conjunction with the PDF evaluated with ``lm::path::pdf_*()`` function, which evaluates the density with aggregated solid angle measure.

.. _path_sampling_aggregated_throughput:

Conversion of aggregated throughput
-------------------------------------

The joint PDF for the primary ray sampling is also bound to be used for the measure conversion.
We can also use the extended geometry term for the conversion:

.. math::

  p_{\mu^*}(\mathbf{x},\omega) G(\mathbf{x}, \mathbf{y}) =
  p_{A^{2*}}(\mathbf{x},\mathbf{y}) :=
  \begin{cases}
    p_{\sigma}(\omega_{\mathbf{x}\to\mathbf{y}}) p_A(\mathbf{y}\mid\omega_{\mathbf{x}\to\mathbf{y}})    & \mathbf{x}\in\mathcal{S}_{\mathrm{inf}} \\
    p_A(\mathbf{x}) p_A(\mathbf{y}\mid\mathbf{x})   & \mathbf{x}\notin\mathcal{S}_{\mathrm{inf}}.
  \end{cases}

Note that in the case of :math:`\mathbf{x}\in\mathcal{S}_{\mathrm{inf}}`, the conversion happens for the projected area measure, since the differential area around :math:`\mathbf{x}` are orthogonally projected to the surface around :math:`\mathbf{y}`:

.. math::

  p_{A}(\mathbf{x})
    = \underbrace{ \left| \mathbf{n}(\mathbf{x}) \cdot \omega_{\mathbf{x} \to \mathbf{y}} \right| }_{G(\mathbf{x},\mathbf{y})} p_{A^\bot}(\mathbf{y}).

We also note that in this case the converted measure is *not* a product area measure :math:`dA^2` but :math:`d\sigma dA`. We denote the converted aggregate measure as :math:`dA^{2*}`.

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
    * - :math:`\bar{x}_L` or :math:`\bar{y}`
      - Light subpath
      - ``Path``
    * - :math:`\bar{x}_E` or :math:`\bar{z}`
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

.. _path_sampling_light_transport_path:

Light transport path
-------------------------------------

A *path* :math:`\bar{x}` is defined by a sequence of path vertices.
We denote the path with the number of vertices :math:`k` as :math:`\bar{x}_k:=\mathbf{x}_0\mathbf{x}_2\dots\mathbf{x}_{k-1}`.
We often omit the subscript :math:`k` depending on the context.

- A path is *full path* if the path constitutes of a complete light transport path, where :math:`\mathbf{x}_0\in\mathcal{M}_L`, :math:`\mathbf{x}_{k-1}\in\mathcal{M}_E`. In the context without ambiguity, we call a *full path* as merely a *path*.
- A path is *subpath* if the path starts but not ends its vertices on the endpoints. Note that the subpath always starts from an endpoint, irrespective to the type of the endpoint. If :math:`\mathbf{x}_0\in\mathcal{M}_L`, the subpath is called *light subpath*. If :math:`\mathbf{x}_0\in\mathcal{M}_E`, the subpath is called *eye subpath*.

In the framework, :cpp:class:`lm::Path` structure represents a path, which holds a vector ``.vs``  of :cpp:class:`lm::Vert` representing a path vertex.
A path vertex structure is a tuple of a surface interaction ``.sp`` and an integer ``.comp`` representing the component index associated to the scene interaction.
We denote the component index associated to the vertex :math:`\mathbf{x}_i` as :math:`j_i`.

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
      - :cpp:func:`lm::Path::vertex_at` with ``trans_dir = LE``
    * - :math:`\mathbf{x}_{k-1-i}`
      - :math:`i`-th path vertex from :math:`\mathbf{x}_{k-1}`
      - :cpp:func:`lm::Path::vertex_at` with ``trans_dir = EL``
    * - :math:`\mathbf{x}_i`
      - :math:`i`-th path vertex from :math:`\mathbf{x}_0`
      - :cpp:func:`lm::Path::subpath_vertex_at`

.. note::

  :cpp:func:`lm::Path::vertex_at` or :cpp:func:`lm::Path::subpath_vertex_at` returns
  a pointer to a path vertex and ``nullptr`` if the index is out of bound,
  which is intentional to simplify the implementation.

.. _path_sampling_sampling_subpath:

Sampling subpath
-------------------------------------

:cpp:func:`lm::path::sample_subpath` function samples a subpath up to the given maximum number of vertices ``max_verts``. The type of subpath can be configured by the argument ``trans_dir``. In math notations, this process can be written as

.. math::

  \bar{x} = \mathbf{x}_{0}\mathbf{x}_{1}\dots\mathbf{x}_{l-1} \sim
  \begin{cases}
    p_E(\cdot)  & \text{if transport direction is } E\to L \\
    p_L(\cdot)  & \text{if transport direction is } L\to E,
  \end{cases}

where :math:`l` is the maximum number of vertices.
In fact, each vertex is sampled sequentially

.. math::

  (\mathbf{x}_0, \mathbf{x}_1) &\sim p_{A^{2*}\Sigma}(\cdot,\cdot), \\
  j_0 &\sim p_{c}(\cdot\mid\mathbf{x}_0), \\
  j_1 &\sim p_{c}(\cdot\mid\mathbf{x}_1), \\
  \mathbf{x}_i &\sim p_{A\to}(\cdot\mid\mathbf{x}_{i-1},j_{i-1}), \\
  j_i &\sim p_{c}(\cdot\mid\mathbf{x}_i)

where :math:`i=2,\dots,(l-1)` and :math:`\Sigma\in\{ L,E \}`. Thus the PDF for subpath sampling can be written as

.. math::

  p_\Sigma(\bar{x}) =
    p_{A^{2*}\Sigma}(\mathbf{x}_0,\mathbf{x}_1)
    p_{c}(j_0\mid\mathbf{x}_0)
    p_{c}(j_1\mid\mathbf{x}_1)
    \prod_{i=2}^{l-1}
      p_{A\to}(\mathbf{x}_{i}\mid\mathbf{x}_{i-1})
      p_{c}(j_i\mid\mathbf{x}_i).

The above equation abstracts the actual sampling process which iteratively samples directions and applies ray casting to find the next intersected points:

.. math::

  \begin{alignat}{3}
    (\mathbf{x}_0, \omega_0) &\sim p_{\mu^* \Sigma}(\cdot,\cdot),\\
    \mathbf{x}_1 &= \mathbf{x}_\mathcal{M}(\mathbf{x}_0, \omega_0), \\
    \omega_i &\sim p_{\sigma^* \to}(\cdot\mid\mathbf{x}_{i-1},j_{i-1}), \\
    \mathbf{x}_i &= \mathbf{x}_\mathcal{M}(\mathbf{x}_{i-1}, \omega_i).
  \end{alignat}


Note that the first two vertices are always sampled from the joint distribution.
If :math:`\mathbf{x}_0` and :math:`\mathbf{x}_1` are independent, the sampling process is equivalent to

.. math::

  \mathbf{x}_0 &\sim p_{A\Sigma}(\cdot), \\
  \mathbf{x}_i &\sim p_{A\to}(\cdot\mid\mathbf{x}_{i-1},j_{i-1}),

where :math:`i=1,\dots,(l-1)`.

.. note::

  The number of vertiecs sampled with :cpp:func:`lm::path::sample_subpath` function might not always same as ``max_verts`` due to the early termination of the subpath. The early termination happens for instance when the ray doesn't hit with any objects before sampling ``max_verts`` vertices.

.. note::

  For simplicity, we don't use Russian roulette in :cpp:func:`lm::path::sample_subpath` function.
  You need to define your own subpath sampling function to support that.

Sampling subpath from endpoint
-------------------------------------

:cpp:func:`sample_subpath_from_endpoint` function can continue to sample the path vertices from the last vertex of the existing subpath. If the given path is empty, this function is equivalent to :cpp:func:`lm::path::sample_subpath`.

.. _path_sampling_connecting_subpaths:

Connecting subpaths
-------------------------------------

:cpp:func:`lm::path::connect_subpaths` function can combine light subpath :math:`\bar{y}` and eye subpath :math:`\bar{z}` with a given number of vertices :math:`s` and :math:`t` in each subpath respectively. This process amounts to sampling a full path with the strategy :math:`(s,t)`. If the subpaths are not *connectable*, the connection process will be failed and the function returns ``std::nullopt``.
For the strategy index :math:`(s,t)` where :math:`s+t\geq 1`, the subpaths are connectable according to the following conditions:

- if :math:`s=0`, :math:`\mathbf{z}_{t-1}\notin\mathcal{S}_{\mathrm{deg}}`,
- if :math:`t=0`, :math:`\mathbf{y}_{s-1}\notin\mathcal{S}_{\mathrm{deg}}`,
- if :math:`s=1`, :math:`\mathbf{y}_0\in\mathcal{S}_{\mathrm{conn}}`  and :math:`\mathbf{z}_{t-1}\notin\mathcal{S}_{\mathrm{spec}}` and :math:`V(\mathbf{y}_{0}, \mathbf{z}_{t-1}) = 0`,
- if :math:`t=1`, :math:`\mathbf{z}_0\in\mathcal{S}_{\mathrm{conn}}` and :math:`\mathbf{y}_{s-1}\notin\mathcal{S}_{\mathrm{spec}}` and :math:`V(\mathbf{y}_{s-1}, \mathbf{z}_{0}) = 0`,
- if :math:`s>0` and :math:`t>0`, :math:`\mathbf{y}_{s-1}\notin\mathcal{S}_{\mathrm{spec}}` and :math:`\mathbf{z}_{t-1}\notin\mathcal{S}_{\mathrm{spec}}` and :math:`V(\mathbf{y}_{s-1}, \mathbf{z}_{t-1}) = 0`.

The vertex :math:`\mathbf{x}` is *specular* if the directional component includes a delta function.
We use the notation :math:`\mathbf{x}\in\mathcal{S}_{\mathrm{spec}}` to denote the property of the vertex. 
For instance, perfect specular reflection is specular. 
A specular vertex needs special treatment since its support cannot be sampled without deterministic selection.
This condition can be checked using :cpp:func:`lm::path::is_specular_component` function.

The endpoint :math:`\mathbf{x}` is *connectable* if corresponding positional and directional PDFs can be evaluated independently. We use the notation :math:`\mathbf{x}\in\mathcal{S}_{\mathrm{conn}}`. For instance, the endpoint is connectable if the first two subpath vertices are independent. This condition can be checked using :cpp:func:`lm::path::connectable_endpoint` function.

Given an strategy :math:`s`, we call the subpaths satisfying the above condition being *connectable subpaths by the strategy* :math:`s` and the connected full path being *samplable by the strategy* :math:`s`. 
This condition can be checked by :cpp:func:`lm::Path::is_samplable_bidir` function,
assuming the visibility function in the definition is assumed to be one.

.. note::

  An endpoint can be connectable even if :math:`\bar{x}_{\Sigma,0}` and :math:`\bar{x}_{\Sigma,1}` are not independent as long as we can compute the marginals of the joint distribution analytically.

.. _path_sampling_measurement_contribution:

Evaluating measurement contribution
-----------------------------------------------

:cpp:func:`lm::Path::eval_measurement_contrb_bidir` function evaluates the *measurement contribution function* defined by

.. math::

  f_{s,t}(\bar{x}) &= f_L(\bar{y}) c_{s,t}(\bar{y}, \bar{z}) f_E(\bar{z}),  \\
  f_L(\bar{y}) &=
    \begin{cases}
      1   & s = 0 \\
      \prod_{i=0}^{s-2} f_{sL}(\mathbf{y}_{i-1},\mathbf{y}_{i},\mathbf{y}_{i+1})
                          G(\mathbf{y}_{i},\mathbf{y}_{i+1})
        & \text{otherwise},
    \end{cases} \\
  f_L(\bar{z}) &=
    \begin{cases}
      1   & t = 0 \\
      \prod_{i=0}^{t-2} f_{sE}(\mathbf{z}_{i-1},\mathbf{z}_{i},\mathbf{z}_{i+1})
                          G(\mathbf{z}_{i},\mathbf{z}_{i+1})
        & \text{otherwise},
    \end{cases} \\
  c_{s,t}(\bar{y}, \bar{z}) &=
    \begin{cases}
      f_{sL}(\mathbf{y}_0,\mathbf{y}_1)   & s = 0 \\
      f_{sE}(\mathbf{z}_0,\mathbf{z}_1)   & t = 0 \\
      f_{sL}(\mathbf{y}_{s-2},\mathbf{y}_{s-1},\mathbf{y}_{t-1})
      G(\mathbf{x}_{t-1}, \mathbf{x}_{s-1})
      f_{sE}(\mathbf{z}_{t-2},\mathbf{z}_{t-1},\mathbf{z}_{s-1})
        & \text{otherwise}.
    \end{cases}

Here, :math:`c_{s,t}(\bar{y}, \bar{z})` is called the *connection term*, which can be evaluated by :cpp:func:`lm::Path::eval_connection_term` function. 

.. note::

  The original measurement contribution function does not take strategy index
  since the scattering term :math:`f_s` is assumed to be symmetrical. 
  This difference comes from the handling of asymmetric scattering in :cpp:func:`lm::path::eval_contrb_direction` function.

.. _path_sampling_bidirectional_path_pdf:

Evaluating bidirectional path PDF
-------------------------------------

:cpp:func:`lm::Path::pdf_bidir` function evaluates the *bidirectional path PDF* defined by

.. math::

  p_{s,t}(\bar{x}) &=
    \begin{cases}
      p_L(\bar{y}) p_E(\bar{z})
        & \bar{x} \text{ is samplable by the strategy } (s,t) \\
      0 
        & \text{otherwise},
    \end{cases} \\
  p_L(\bar{y}) &=
    \begin{cases}
      1
          & s = 0 \\
      p_{AL}(\mathbf{y}_0)
          & s = 1 \\
      p_{A^{2*}L}(\mathbf{y}_0, \mathbf{y}_1)
        \prod_{i=2}^{s-1} p_{A\to}(\mathbf{y}_i\mid\mathbf{y}_{i-1}) 
          & s > 1,
    \end{cases}\\
  p_E(\bar{z}) &=
    \begin{cases}
      1
          & t = 0 \\
      p_{AE}(\mathbf{z}_0)
          & t = 1 \\
      p_{A^{2*}E}(\mathbf{z}_0, \mathbf{z}_1)
        \prod_{i=2}^{t-1} p_{A\to}(\mathbf{z}_i\mid\mathbf{z}_{i-1}) 
          & t > 1.
    \end{cases}

.. _path_sampling_evaluating_sampling_weight:

Evaluating sampling weight
-------------------------------------

For convenience, the path structure provides a function :cpp:func:`lm::Path::eval_sampling_weight_bidir` to compute sampling weight :math:`f_{s,t}(\bar{x})/p_{s,t}(\bar{x})`.  We assume the input of the function is samplable by the strategy :math:`(s,t)`. Specifically, the sampling weight is defined as

.. math::

  C^*_{s,t}(\bar{x}) &= \alpha_L(\bar{y}) c_{s,t}(\bar{y}, \bar{z}) \alpha_E(\bar{z}), \\
  \alpha_L(\bar{y}) &=
    \begin{cases}
      1
        & s = 0 \\
      \frac{1}{p_{AL}(\mathbf{y}_0)}
        & s = 1 \\
      \frac{f_{sL}(\mathbf{y}_0,\mathbf{y}_1)}{p_{A^{2*}L}(\mathbf{y}_0, \mathbf{y}_1)}
        \prod_{i=2}^{s-1}
          \frac{f_{sL}(\mathbf{y}_{i-1},\mathbf{y}_{i},\mathbf{y}_{i+1})}{p_{\sigma^*\to}(\mathbf{y}_i\mid\mathbf{y}_{i-1})}
        & s > 1,
    \end{cases} \\
  \alpha_E(\bar{z}) &=
    \begin{cases}
      1
        & t = 0 \\
      \frac{1}{p_{AL}(\mathbf{z}_0)}
        & t = 1 \\
      \frac{f_{sL}(\mathbf{z}_0,\mathbf{z}_1)}{p_{A^{2*}L}(\mathbf{z}_0, \mathbf{z}_1)}
        \prod_{i=2}^{t-1}
          \frac{f_{sL}(\mathbf{z}_{i-1},\mathbf{z}_{i},\mathbf{z}_{i+1})}{p_{\sigma^*\to}(\mathbf{z}_i\mid\mathbf{z}_{i-1})}
        & t > 1,
    \end{cases}

where :math:`\alpha_L(\bar{y})` or :math:`\alpha_E(\bar{z})` is called *subpath sampling weight*, which can be computed by :cpp:func:`lm::Path::eval_subpath_sampling_weight` function. 

.. _path_sampling_mis_weight:

Evaluating MIS weight
-------------------------------------

:cpp:func:`lm::Path::eval_mis_weight` function evaluates MIS weight via power heuristic. 

Summary of notations
===========================

The path sampling interface defines various properties associated to the point to the scene :math:`\mathbf{x}`. The following table summarizes the notations introduced in this section.

.. list-table::
    :widths: 20 40 40
    :header-rows: 1

    * - Notation
      - Description
      - Code

    * - :math:`\mathbf{x} \in \mathcal{M}_E`
      - :math:`\mathbf{x}` is :ref:`camera endpoint<path_sampling_scene_interaction_type>`
      - ``sp.is_type(CameraEndpoint)``
    * - :math:`\mathbf{x} \in \mathcal{M}_L`
      - :math:`\mathbf{x}` is :ref:`light endpoint<path_sampling_scene_interaction_type>`
      - ``sp.is_type(LightEndpoint)``
    * - :math:`\mathbf{x} \in \mathcal{M}_S`
      - :math:`\mathbf{x}` is :ref:`surface interaction<path_sampling_scene_interaction_type>`
      - ``sp.is_type(SurfaceInteraction)``
    * - :math:`\mathbf{x} \in \mathcal{V}`
      - :math:`\mathbf{x}` is :ref:`medium interaction<path_sampling_scene_interaction_type>`
      - ``sp.is_type(MediumInteraction)``
    * - :math:`\mathbf{x}\in\mathcal{S}_{\mathrm{deg}}`
      - :math:`\mathbf{x}` is :ref:`degenerated<path_sampling_degenerated_point>`
      - ``geom.degenerated``
    * - :math:`\mathbf{x}\in\mathcal{S}_{\mathrm{inf}}`
      - :math:`\mathbf{x}` is :ref:`infinitely distant point<path_sampling_infinitely_distant_point>`
      - ``geom.infinite``
    * - :math:`\mathbf{x}\in\mathcal{S}_{\mathrm{spec}}`
      - :math:`\mathbf{x}` is :ref:`specular<path_sampling_specular_vertex>`
      - ``lm::path::is_specular_component(sp,comp)``
    * - :math:`\mathbf{x}\in\mathcal{S}_{\mathrm{conn}}`
      - :math:`\mathbf{x}` is :ref:`connectable endpoint<path_sampling_connectable_endpoint>`
      - ``lm::path::connectable_endpoint(sp)``