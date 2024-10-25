project('eagle_eye', 'cpp')

# Import gnome module for resource compilation
gnome = import('gnome')

gtkmm_dep = dependency('gtkmm-3.0', version: '>= 3.24.0')

executable('ee', 
    ['main.cpp', 'main_window.cpp'],
    install_dir: join_paths(get_option('prefix'), 'bin', 'eagle_eye'),
    dependencies: gtkmm_dep
)