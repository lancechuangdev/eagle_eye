project('eagle_eye', 'cpp')

# Import gnome module for resource compilation
gnome = import('gnome')

# -------------------------------
# Dependencies
# -------------------------------

# Core dependencies
gtkmm_dep = dependency('gtkmm-3.0', version: '>= 3.24.0')
opencv_dep = dependency('opencv4', required: true)
boost_dep = dependency('boost', required: true)
openssl_dep = dependency('openssl', required: true)
json_dep = dependency('nlohmann_json', required: true)

# Camera SDK paths
mvcam_runenv = '/opt/MVS/lib'
mvcam_inc_dir = join_paths(mvcam_runenv, '../include')
mvcam_lib_dir = join_paths(mvcam_runenv, '64')

# ONNX Runtime GPU Dependency
onnxruntime_dir = '/home/boris-alienware/Downloads/onnxruntime-linux-x64-gpu-1.20.1'
onnxruntime_inc_dir = include_directories(join_paths(onnxruntime_dir, 'include'))
onnxruntime_lib_dir = join_paths(onnxruntime_dir, 'lib')
onnxruntime_dep = declare_dependency(
    include_directories: onnxruntime_inc_dir,
    link_args: ['-L' + onnxruntime_lib_dir, '-lonnxruntime']
)

# -------------------------------
# Resource Compilation
# -------------------------------

# Generate the binary resource file, no need to install it directly
resource_target = gnome.compile_resources(
    'eagle_eye_resources',
    'eagle_eye_gresource.xml',
    source_dir: 'icons',  # Directory where your icons are located
)

# -------------------------------
# Executable
# -------------------------------

executable('ee', 
    [resource_target, 'main.cpp', 'main_window.cpp', 'app_paths.cpp', 'frame_queue.cpp', 'pixbuf_queue.cpp', 'file_utils.cpp', 'frame_utils.cpp','time_utils.cpp', 'logger.cpp', 'retention_manager.cpp', 'settings_service.cpp'],
    install: true,
    install_dir: join_paths(get_option('prefix'), 'bin', 'eagle_eye'),
    dependencies: [gtkmm_dep, opencv_dep, boost_dep, openssl_dep, json_dep, onnxruntime_dep],
    include_directories: [mvcam_inc_dir, onnxruntime_inc_dir],
    link_args: [
        '-Wl,-rpath=' + mvcam_lib_dir,
        '-L' + mvcam_lib_dir,
        '-lMvCameraControl'
    ]
)

# -------------------------------
# Installations
# -------------------------------

# Install application data files
install_data(
    ['ui.glade', 'style.css', 'model.ckpt', 'model.onnx'],
    install_dir: join_paths(get_option('prefix'), 'share', 'eagle_eye')
)

# Install app icon to the system icons directory
install_data(
    'com.example.eagle_eye.png',
    install_dir: '/usr/share/icons/hicolor/512x512/apps',
)

# Install .desktop file to system-wide applications directory
install_data(
    'eagle_eye.desktop',
    install_dir: '/usr/share/applications'
)