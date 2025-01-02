project('eagle_eye', 'cpp')

# Import gnome module for resource compilation
gnome = import('gnome')

gtkmm_dep = dependency('gtkmm-3.0', version: '>= 3.24.0')

# Add Boost and WebSocket++ dependencies
boost_dep = dependency('boost', required: true)
openssl_dep = dependency('openssl', required: true)

# Add JSON dependency
json_dep = dependency('nlohmann_json', required: true)

# WebSocket++ path
websocketpp_inc_dir = include_directories('/usr/include/websocketpp')

# Retrieve the value of the environment variable
mvcam_runenv = '/opt/MVS/lib'

# Add include directory and library paths using the environment variable
inc_dir = join_paths(mvcam_runenv, '../include')
lib_dir = join_paths(mvcam_runenv, '64')

# Generate the binary resource file, no need to install it directly
resource_target = gnome.compile_resources(
    'eagle_eye_resources',
    'eagle_eye_gresource.xml',
    source_dir: 'icons',  # Directory where your icons are located
)

executable('ee', 
    [resource_target, 'main.cpp', 'main_window.cpp', 'app_paths.cpp', 'frame_queue.cpp', 'file_utils.cpp', 'time_utils.cpp', 'logger.cpp', 'web_socket_client.cpp', 'retention_manager.cpp', 'settings_service.cpp', 'report_window.cpp'],
    install: true,
    install_dir: join_paths(get_option('prefix'), 'bin', 'eagle_eye'),
    dependencies: [gtkmm_dep, boost_dep, openssl_dep, json_dep],
    include_directories: [inc_dir, websocketpp_inc_dir],
    link_args: [
        '-Wl,-rpath=' + lib_dir,
        '-L' + lib_dir,
        '-lMvCameraControl'
    ]
)

# Install data files
install_data(
    ['ui.glade', 'style.css', 'ds.keras', 'ee_anomaly_detection_service.py'],
    install_dir: join_paths(get_option('prefix'), 'share', 'eagle_eye')
)