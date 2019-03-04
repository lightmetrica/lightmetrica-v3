"""Performance test of acceleration structures"""

import os
import lmfunctest as ft
import lightmetrica as lm
import lmscene
import timeit

def run():
    env = ft.loadenv('.lmenv')

    # Initialize the framework
    lm.init('user::default', {
        'numThreads': -1
    })
    lm.log.setSeverity(lm.log.LogLevel.Warn)

    # Load plugins
    lm.comp.detail.loadPlugin(os.path.join(env.bin_path, 'accel_nanort'))

    # Execute rendering for each scene
    for name in lmscene.scenes():
        for accel in ft.accels():
            print('{} {}'.format(name, accel))
            lm.reset()

            lmscene.load(env.scene_path, name)
            def build():
                lm.build('accel::' + accel, {})
            build_time = timeit.timeit(stmt=build, number=1)
            
            def render():
                lm.render('renderer::raycast', {
                    'output': 'film_output'
                })
            render_time = timeit.timeit(stmt=render, number=1)

            print('build_time: ', render_time)
            print('render_time: ', render_time)

if __name__ == '__main__':
    run()