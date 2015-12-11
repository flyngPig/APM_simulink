#!/usr/bin/env python
# encoding: utf-8

"""
gbenchmark is a Waf tool for benchmark builds in Ardupilot
"""

from waflib import Build, Context, Task
from waflib.TaskGen import feature, before_method, after_method

def configure(cfg):
    env = cfg.env
    env.HAS_GBENCHMARK = False

    cfg.start_msg('Checking for gbenchmark submodule')
    cmake_lists = cfg.srcnode.find_resource('modules/gbenchmark/CMakeLists.txt')
    if not cmake_lists:
        cfg.end_msg('not initialized', color='YELLOW')
        return
    cfg.end_msg('yes')

    cfg.find_program('cmake', mandatory=False)

    if not env.CMAKE:
        return

    env.GBENCHMARK_CMAKE_GENERATOR = None

    cfg.find_program('ninja', mandatory=False)
    if not env.NINJA:
        cfg.find_program('ninja-build', var='NINJA', mandatory=False)

    if env.NINJA:
        env.GBENCHMARK_CMAKE_GENERATOR = 'Ninja'

    env.GBENCHMARK_GENERATOR_OPTION = ''
    if env.GBENCHMARK_CMAKE_GENERATOR:
        env.GBENCHMARK_GENERATOR_OPTION = '-G%s' % env.GBENCHMARK_CMAKE_GENERATOR

    prefix_node = cfg.bldnode.make_node('gbenchmark')
    my_build_node = cfg.bldnode.make_node('gbenchmark_build')
    my_src_node = cfg.srcnode.find_dir('modules/gbenchmark')

    env.GBENCHMARK_PREFIX_REL = prefix_node.path_from(cfg.bldnode)
    env.GBENCHMARK_BUILD = my_build_node.abspath()
    env.GBENCHMARK_BUILD_REL = my_build_node.path_from(cfg.bldnode)
    env.GBENCHMARK_SRC = my_src_node.abspath()

    env.INCLUDES_GBENCHMARK = [prefix_node.make_node('include').abspath()]
    env.LIBPATH_GBENCHMARK = [prefix_node.make_node('lib').abspath()]
    env.LIB_GBENCHMARK = ['benchmark']

    env.HAS_GBENCHMARK = True

class gbenchmark_build(Task.Task):
    def __init__(self, *k, **kw):
        super(gbenchmark_build, self).__init__(*k, **kw)

        bldnode = self.generator.bld.bldnode
        output_list = [
            '%s/%s' % (self.env.GBENCHMARK_PREFIX_REL, path)
            for path in (
                'include/benchmark/benchmark.h',
                'include/benchmark/macros.h',
                'include/benchmark/benchmark_api.h',
                'include/benchmark/reporter.h',
                'lib/libbenchmark.a',
            )
        ]
        self.outputs.extend([bldnode.make_node(f) for f in output_list])

    def run(self):
        bld = self.generator.bld
        cmds = []

        cmake_lists = bld.srcnode.find_resource('modules/gbenchmark/CMakeLists.txt')
        if not cmake_lists:
            bld.fatal('Submodule gbenchmark not initialized, please run configure again')

        # Generate build system first, if necessary
        my_build_node = bld.bldnode.find_dir(self.env.GBENCHMARK_BUILD_REL)
        if not (my_build_node and my_build_node.find_resource('CMakeCache.txt')):
            if not my_build_node:
                bld.bldnode.make_node(self.env.GBENCHMARK_BUILD_REL).mkdir()

            cmds.append('%s %s -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=%s %s' % (
                            self.env.CMAKE[0],
                            self.env.GBENCHMARK_SRC,
                            bld.bldnode.make_node(self.env.GBENCHMARK_PREFIX_REL).abspath(),
                            self.env.GBENCHMARK_GENERATOR_OPTION
                        ))

        cmds.append('%s --build %s --target install' % (
                      self.env.CMAKE[0],
                      self.env.GBENCHMARK_BUILD
                    ))
        try:
            for cmd in cmds:
                bld.cmd_and_log(
                    cmd,
                    cwd=self.env.GBENCHMARK_BUILD,
                    quiet=Context.BOTH,
                )
            return 0
        except Exception as e:
            print(e.stdout, e.stderr)
            return 1

    def __str__(self):
        return 'Google Benchmark'

gbenchmark_build = Task.always_run(Task.update_outputs(gbenchmark_build))

build_task = None

@feature('gbenchmark')
@before_method('process_use')
def append_gbenchmark_use(self):
    self.use = self.to_list(getattr(self, 'use', []))
    if 'GBENCHMARK' not in self.use:
        self.use.append('GBENCHMARK')

@feature('gbenchmark')
@after_method('process_source')
def wait_for_gbenchmark_build(self):
    global build_task

    if not build_task:
        build_task = self.create_task('gbenchmark_build')

    for task in self.compiled_tasks:
        task.set_run_after(build_task)
        task.dep_nodes.extend(build_task.outputs)
