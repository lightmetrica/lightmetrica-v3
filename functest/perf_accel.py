"""Performance test of acceleration structures
"""

import os
import lmenv
env = lmenv.load('.lmenv')
import lightmetrica as lm
import scene
import timeit

def run():
    lm.init('user::default', {
        'numThreads': -1
    })
    lm.log.setSeverity(lm.log.LogLevel.Warn)

    # Load plugins
    lm.comp.detail.loadPlugin(os.path.join(env.bin_path, 'accel_nanort'))

    # Output directory
    # outdir = './output/perf_accel'
    # if not os.path.exists(outdir):
    #     os.makedirs(outdir)

    # Execute rendering for each scene
    for name in scene.scenes():
        for accel in ['sahbvh', 'nanort']:
            print('{} {}'.format(name, accel))
            lm.reset()

            scene.load(env.scene_path, name)
            lm.build('accel::' + accel, {})
            
            def render():
                lm.render('renderer::raycast', {
                    'output': 'film_output'
                })
            render_time = timeit.timeit(stmt=render, number=1)

            #lm.save('film_output', os.path.join(outdir, '{}_{}.hdr'.format(name, accel)))
            #lm.save('film_output', os.path.join(outdir, '{}_{}.png'.format(name, accel)))
            
            print('render_time: ', render_time)

if __name__ == '__main__':
    run()