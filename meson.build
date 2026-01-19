project('funnel-test', 'c')

includes = include_directories('include')

pipewire = dependency('libpipewire-0.3')
gbm = dependency('gbm')
egl = dependency('egl')

lib_funnel = library('funnel', 'src/funnel.c',
    dependencies: [gbm, egl, pipewire],
    include_directories : includes)
funnel = declare_dependency(link_with : lib_funnel, include_directories : includes)

lib_funnel_egl = library('funnel-egl', 'src/egl.c', dependencies: [funnel, pipewire, egl])
lib_funnel_vk = library('funnel-vk', 'src/vulkan.c', dependencies: [funnel, pipewire])

funnel_egl = declare_dependency(link_with : lib_funnel_egl, include_directories : includes)
funnel_vk = declare_dependency(link_with : lib_funnel_vk, include_directories : includes)

gl = dependency('GL')
x11 = dependency('x11')
executable('funnel-test-egl', 'test-egl.c', dependencies: [gl, egl, x11, funnel, funnel_egl])
