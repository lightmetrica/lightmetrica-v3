import lightmetrica as lm
import argparse

def run(**kwargs):
    lm.init('user::default', {})
    lm.info()
    lm.net.worker.init('net::worker::default', {
        'name': kwargs['name'],
        'port': kwargs['port']
    })
    lm.net.worker.run()

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--name', type=str, required=True)
    parser.add_argument('--port', type=int, required=True)
    args = parser.parse_args()
    run(**vars(args))