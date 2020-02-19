.. _path_sampling:

Path sampling
######################

In this section we will discuss about path sampling API of Lightmetrica.
Implementing a rendering technique means you want to implement path sampling technique
and evaluating the contribution and probability that the path is being sampled.
Lightmetrica provides an API for path sampling and evaluation which supports typical use-cases to implement various rendering techniques. 

.. note::

    The math notations in this page is based on a seminal PhD thesis by Veach [1998], although there are slight differences.

Notations
===========================

.. list-table::
    :widths: 20 80
    :header-rows: 1

    * - Symbol
      - Description
    * - :math:`\mathbf{x}`
      - Position in a scene
    * - :math:`\omega`
      - Direction
    * - :math:`dA`
      - Area measure
    * - :math:`d\sigma`
      - Solid angle measure
    * - :math:`d\sigma^\bot`
      - Projected solid angle measure
    * - :math:`\bar{x}`
      - Light transport path (or just path)
    * - :math:`(s,t)`
      - Strategy index of path sampling
    * - :math:`s`
      - Number of vertices in light subpath
    * - :math:`t`
      - Number of vertices in eye subpath

Scene interaction
================================

:cpp:class:`lm::SceneInteraction`

Represents a point :math:`\mathbf{x}` in the scene.
A type of scene interaction is either of the following:

- :math:`\mathbf{x} \in \mathcal{M}_E`: *camera endpoint*
- :math:`\mathbf{x} \in \mathcal{M}_L`: *light endpoint*
- :math:`\mathbf{x} \in \mathcal{M}`: *surface interaction*
- :math:`\mathbf{x} \in \mathcal{V}`: *medium interaction*

Local sampling / evaluation
================================

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

:cpp:func:`lm::path::eval_contrb_direction`

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


Global path sampling
===========================

Path integral
-------------------------------------

.. math::

  I_j = \int_\mathcal{P} f_j(\bar{x}) d\bar{x}