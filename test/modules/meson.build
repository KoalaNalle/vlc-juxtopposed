vlc_tests += {
    'name': 'test_modules_demux_wav_timing',
    'sources': files('demux/wav_timing.c'),
    'suite': ['modules', 'test_modules'],
    'link_with': [libvlc, libvlccore],
    'module_depends': ['wav'],
}

vlc_tests += {
    'name' : 'test_modules_access_output_file',
    'sources' : files('access_output/file.c'),
    'suite' : ['modules', 'test_modules'],
    'link_with' : [libvlc, libvlccore],
    'module_depends' : ['access_output_file'],
}

vlc_tests += {
    'name' : 'test_modules_lua_extension',
    'sources' : files('lua/extension.c'),
    'suite' : ['modules', 'test_modules'],
    'link_with' : [libvlc, libvlccore],
    'module_depends' : vlc_plugins_targets.keys()
}

vlc_tests += {
    'name' : 'test_modules_lua_playlist_parser',
    'sources' : files('lua/playlist_parser.c'),
    'suite' : ['modules', 'test_modules'],
    'link_with' : [libvlc, libvlccore],
    'module_depends' : ['lua'],
}

if not (host_system == 'windows') # missing mkdtemp()
vlc_tests += {
    'name' : 'test_modules_misc_medialibrary',
    'sources' : files('misc/medialibrary.c'),
    'suite' : ['modules', 'test_modules'],
    'link_with' : [libvlc, libvlccore],
    'module_depends' : vlc_plugins_targets.keys(),
    #'module_depends' : ['medialibrary', 'demux_mock', 'jpeg', 'png', 'rawvid'],
}
endif

vlc_tests += {
    'name' : 'test_modules_packetizer_helpers',
    'sources' : files('packetizer/helpers.c'),
    'suite' : ['modules', 'test_modules'],
    'link_with' : [libvlc, libvlccore],
    'module_depends' : vlc_plugins_targets.keys()
}

vlc_tests += {
    'name' : 'test_modules_packetizer_hxxx',
    'sources' : files('packetizer/hxxx.c'),
    'suite' : ['modules', 'test_modules'],
    'link_with' : [libvlc, libvlccore],
    'module_depends' : vlc_plugins_targets.keys()
}

vlc_tests += {
    'name' : 'test_modules_packetizer_h264',
    'sources' : files('packetizer/h264.c'),
    'suite' : ['modules', 'test_modules'],
    'link_with' : [libvlc, libvlccore],
    'module_depends' : vlc_plugins_targets.keys()
}

vlc_tests += {
    'name' : 'test_modules_packetizer_hevc',
    'sources' : files('packetizer/hevc.c'),
    'suite' : ['modules', 'test_modules'],
    'link_with' : [libvlc, libvlccore],
    'module_depends' : vlc_plugins_targets.keys()
}

vlc_tests += {
    'name' : 'test_modules_packetizer_h266',
    'sources' : files('packetizer/h266.c'),
    'suite' : ['modules', 'test_modules'],
    'link_with' : [libvlc, libvlccore],
    'module_depends' : vlc_plugins_targets.keys()
}

vlc_tests += {
    'name' : 'test_modules_packetizer_h266_nal',
    'sources' : files(
        'packetizer/h266_nal.c',
        '../../modules/packetizer/h266_nal.c'
      ),
    'suite' : ['modules', 'test_modules'],
    'link_with' : [libvlc, libvlccore],
    'module_depends' : vlc_plugins_targets.keys()
}

vlc_tests += {
    'name' : 'test_modules_packetizer_copy',
    'sources' : files('packetizer/copy.c'),
    'suite' : ['modules', 'test_modules'],
    'link_with' : [libvlc, libvlccore],
    'module_depends' : vlc_plugins_targets.keys()
}

vlc_tests += {
    'name' : 'test_modules_packetizer_mpegvideo',
    'sources' : files('packetizer/mpegvideo.c'),
    'suite' : ['modules', 'test_modules'],
    'link_with' : [libvlc, libvlccore],
    'module_depends' : vlc_plugins_targets.keys()
}

vlc_tests += {
    'name' : 'test_modules_keystore',
    'sources' : files('keystore/test.c'),
    'suite' : ['modules', 'test_modules'],
    'link_with' : [libvlc, libvlccore],
    'module_depends' : vlc_plugins_targets.keys()
}

if not(host_system == 'windows')
vlc_tests += {
    'name' : 'test_modules_tls',
    'sources' : files('misc/tls.c'),
    'suite' : ['modules', 'test_modules'],
    'link_with' : [libvlc, libvlccore],
    'module_depends' : vlc_plugins_targets.keys()
}
endif

vlc_tests += {
    'name' : 'test_modules_demux_json',
    'sources' : files('demux/json.c'),
    'suite' : ['modules', 'test_modules'],
    'link_with' : [vlc_json_lib, libvlccore],
}

vlc_tests += {
    'name' : 'test_modules_demux_libmp4',
    'sources' : files(
        'demux/libmp4.c',
        '../../modules/demux/mp4/libmp4.c'),
    'suite' : ['modules', 'test_modules'],
    'dependencies' : [z_dep],
    'link_with' : [libvlc, libvlccore],
}

vlc_tests += {
    'name' : 'test_modules_demux_timestamps',
    'sources' : files('demux/timestamps.c'),
    'suite' : ['modules', 'test_modules']
}

vlc_tests += {
    'name' : 'test_modules_demux_timestamps_filter',
    'sources' : files('demux/timestamps_filter.c'),
    'suite' : ['modules', 'test_modules'],
    'link_with' : [libvlc, libvlccore],
    'module_depends' : vlc_plugins_targets.keys()
}

vlc_tests += {
    'name' : 'test_modules_ts_pes',
    'sources' : files(
        'demux/ts_pes.c',
        '../../modules/demux/mpeg/ts_pes.c',
        '../../modules/demux/mpeg/ts_pes.h'),
    'suite' : ['modules', 'test_modules'],
    'link_with' : [libvlc, libvlccore],
    'module_depends' : vlc_plugins_targets.keys()
}

vlc_tests += {
    'name' : 'test_modules_codec_hxxx_helper',
    'sources' : files('codec/hxxx_helper.c'),
    'suite' : ['modules', 'test_modules'],
    'link_with' : [libvlc, libvlccore, hxxxhelper_lib],
    'module_depends' : vlc_plugins_targets.keys()
}

vlc_tests += {
    'name' : 'test_modules_codec_cea708_aspect_ratio',
    'sources' : files('codec/cea708_aspect_ratio.c'),
    'suite' : ['modules', 'test_modules'],
    'link_with' : [libvlc, libvlccore],
    'module_depends' : vlc_plugins_targets.keys()
}

vlc_tests += {
    'name' : 'test_modules_codec_cea708_integration',
    'sources' : files('codec/cea708_integration.c'),
    'suite' : ['modules', 'test_modules'],
    'link_with' : [libvlc, libvlccore],
    'module_depends' : vlc_plugins_targets.keys()
}

vlc_tests += {
    'name' : 'test_modules_codec_cea708',
    'sources' : files('codec/cea708.c',
                      '../../modules/codec/cea708.c'),
    'suite' : ['modules', 'test_modules'],
    'link_with' : [libvlc, libvlccore],
    'module_depends' : vlc_plugins_targets.keys()
}

vlc_tests += {
    'name' : 'test_modules_video_output_opengl_filters',
    'sources' : files(
        'video_output/opengl/filters.c',
        '../../modules/video_output/opengl/filters.c',
        '../../modules/video_output/opengl/filters.h',
        '../../modules/video_output/opengl/filter.c',
        '../../modules/video_output/opengl/gl_api.c',
        '../../modules/video_output/opengl/gl_api.h',
        '../../modules/video_output/opengl/interop.c',
        '../../modules/video_output/opengl/interop.h',
        '../../modules/video_output/opengl/importer.c',
        '../../modules/video_output/opengl/importer.h'),
    'suite' : ['modules', 'test_modules'],
    'link_with' : [libvlc, libvlccore, libvlc_opengl],
    'c_args' : ['-DVLC_TEST_OPENGL_API=VLC_OPENGL'],
    'module_depends' : vlc_plugins_targets.keys(),
    'enabled' : opengl_dep.found(),
}

vlc_tests += {
    'name' : 'test_modules_video_output_opengl_es2_filters',
    'sources' : files(
        'video_output/opengl/filters.c',
        '../../modules/video_output/opengl/filters.c',
        '../../modules/video_output/opengl/filters.h',
        '../../modules/video_output/opengl/filter.c',
        '../../modules/video_output/opengl/gl_api.c',
        '../../modules/video_output/opengl/gl_api.h',
        '../../modules/video_output/opengl/interop.c',
        '../../modules/video_output/opengl/interop.h',
        '../../modules/video_output/opengl/importer.c',
        '../../modules/video_output/opengl/importer.h'),
    'suite' : ['modules', 'test_modules'],
    'link_with' : [libvlc, libvlccore, libvlc_opengles],
    'c_args' : ['-DVLC_TEST_OPENGL_API=VLC_OPENGL_ES2'],
    'module_depends' : vlc_plugins_targets.keys(),
    'enabled' : opengles2_dep.found(),
}

vlc_tests += {
    'name' : 'test_modules_stream_out_transcode',
    'sources' : files(
        'stream_out/transcode.c',
        'stream_out/transcode.h',
        'stream_out/transcode_scenarios.c'),
    'suite' : ['modules', 'test_modules'],
    'link_with' : [libvlc, libvlccore],
    'module_depends' : vlc_plugins_targets.keys()
}

vlc_tests += {
    'name' : 'test_modules_stream_out_sdi',
    'sources' : files(
        'stream_out/sdi.cpp',
        '../../modules/stream_out/sdi/SDIStream.cpp',
        '../../modules/stream_out/sdi/SDIStream.hpp'),
    'suite' : ['modules', 'test_modules'],
    'link_with' : [libvlc, libvlccore]
}

vlc_tests += {
    'name' : 'test_modules_stream_out_pcr_sync',
    'sources' : files(
        'stream_out/pcr_sync.c',
        '../../modules/stream_out/transcode/pcr_sync.c',
        '../../modules/stream_out/transcode/pcr_sync.h',
        '../../modules/stream_out/transcode/pcr_helper.c',
        '../../modules/stream_out/transcode/pcr_helper.h'),
    'suite' : ['modules', 'test_modules'],
    'link_with' : [libvlccore],
    'module_depends' : vlc_plugins_targets.keys()
}

vlc_tests += {
    'name' : 'test_modules_mux_webvtt',
    'sources' : files('mux/webvtt.c'),
    'suite' : ['modules', 'test_modules'],
    'link_with' : [libvlc, libvlccore],
    'module_depends' : vlc_plugins_targets.keys()
}


if soxr_dep.found()
    vlc_tests += {
        'name': 'test_modules_soxr_timestamps',
        'sources': files('audio_filter/soxr_timestamps.c'),
        'suite': ['modules', 'test_modules'],
        'link_with': [libvlc, libvlccore],
        'dependencies': [m_lib],
        'module_depends': ['soxr'],
    }
    soxr_native_test_link_args = []
    soxr_native_test_dependencies = [soxr_dep, m_lib]
    if host_system == 'windows' and cc.get_id() == 'gcc'
        soxr_native_test_link_args += ['-static-libgcc', '-no-pthread']
        soxr_native_test_dependencies += cc.find_library('winpthread', static: true)
    endif
    vlc_tests += {
        'name': 'test_modules_soxr_lifecycle',
        'sources': files('audio_filter/soxr_lifecycle.c'),
        'suite': ['modules', 'test_modules'],
        'link_with': [libvlc, libvlccore],
        'dependencies': soxr_native_test_dependencies,
        'link_args': soxr_native_test_link_args,
    }
endif


flac_native_test_link_args = []
flac_native_test_dependencies = []
if host_system == 'windows' and cc.get_id() == 'gcc'
    flac_native_test_link_args += ['-static-libgcc', '-no-pthread']
    flac_native_test_dependencies += cc.find_library('winpthread', static: true)
endif
vlc_tests += {
    'name': 'test_modules_packetizer_flac_eos',
    'sources': files('packetizer/flac_eos.c'),
    'suite': ['modules', 'test_modules'],
    'link_with': [libvlc, libvlccore],
    'module_depends': ['packetizer_flac'],
    'dependencies': flac_native_test_dependencies,
    'link_args': flac_native_test_link_args,
}

# The companion runner creates fresh userdata and installs/removes a real
# SQLite rejection trigger while this process waits on its control pipe.
# Build explicitly; running the executable alone is not a default suite test.
if medialibrary_dep.found()
    executable('test_modules_medialibrary_history',
        files('misc/medialibrary_history.c'),
        build_by_default: false,
        link_with: [libvlc, libvlccore, vlc_libcompat],
        include_directories: [vlc_include_dirs, include_directories('../../src')],
        dependencies: [libvlccore_deps, opengl_dep],
        c_args: [
            '-DSRCDIR="@0@"'.format(vlc_src_root + '/test/'),
            '-DTOP_BUILDDIR="@0@"'.format(vlc_build_root),
            '-DTOP_SRCDIR="@0@"'.format(vlc_src_root),
        ],
        win_subsystem: 'console')
endif
