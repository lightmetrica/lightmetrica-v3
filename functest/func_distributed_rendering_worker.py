import lightmetrica as lm
import argparse

def run(**kwargs):
    lm.init('user::default', {})
    lm.info()
    lm.dist.worker.init('dist::worker::default', {
        'name': kwargs['name'],
        'address': kwargs['address'],
        'port': kwargs['port'],
        'numThreads': -1
    })
    lm.dist.worker.run()
    lm.dist.shutdown()
    lm.shutdown()

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--name', type=str, required=True)
    parser.add_argument('--address', type=str, required=True)
    parser.add_argument('--port', type=int, required=True)
    args = parser.parse_args()
    run(**vars(args))