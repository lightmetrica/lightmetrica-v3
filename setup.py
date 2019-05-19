from setuptools import setup, find_packages

setup(
    name='lightmetrica',
    version='3.0.0',
    description='A research-oriented renderer',
    author='Hisanari Otsu',
    author_email='hi2p.perim@gmail.com',
    url='https://github.com/hi2p-perim/lightmetrica-v3',
    #packages=['lightmetrica', 'lightmetrica_jupyter'],
    packages=find_packages(),
    license='MIT',
    classifiers=[
        'Development Status :: 3 - Alpha',
        'Intended Audience :: Developers',
        'Topic :: Software Development :: Libraries :: Python Modules',
        'Topic :: Utilities',
        'Programming Language :: C++',
        'Programming Language :: Python :: 3.7',
        'License :: OSI Approved :: MIT License'
    ],
    keywords='Compute graphics, Renderer',
    long_description='Lightmetrica is a research-oriented renderer. The development of the framework is motivated by the goal to provide a practical environment for rendering research and development, where the researchers and developers need to tackle various challenging requirements through the development process.',
)