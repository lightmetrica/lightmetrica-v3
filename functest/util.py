import numpy as np
import json
import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid1 import make_axes_locatable
from scipy.ndimage import gaussian_filter
import lightmetrica as lm
    
# Module-level global variables
err_vmax = 0.1

# -------------------------------------------------------------------------------------------------

def rmse(ref, img):
    """Compute RMSE."""
    return np.sqrt(np.sum((ref-img)**2) / (ref.size))

def rrmse(ref, img):
    """Compute rRMSE."""
    err = rmse(ref, img)
    ave  = np.sum(ref) / (ref.size)
    return err / ave

def rmse_pixelwised(img1, img2):
    """Compute pixelwised RMSE."""
    return np.sqrt(np.sum((img1 - img2) ** 2, axis=2) / 3) 

def diff_positive_negative(image1, image2):
    """Compute positive-negative error image."""
    h, w, _ = image1.shape
    d = image1 - image2
    norm = np.sqrt(np.sum(d**2, axis=2))
    pn = np.sum(d, axis=2)
    diff = np.dstack((
        np.logical_and(norm > 0, pn < 0),
        np.logical_and(norm > 0, pn > 0),
        np.logical_and(norm > 0, pn == 0)
    ))
    return diff.astype(np.float)

# -------------------------------------------------------------------------------------------------

def print_dict(d):
    """Pretty-print a dictionary."""
    print(json.dumps(d, indent=2))
    
def print_errors(ref, img):
    """Compute and prints errors between two images."""
    print('RMSE : %.5f' % rmse(ref, img))
    print('rRMSE: %.5f' % rrmse(ref, img))

def display_image(img, title, fig_size = 20):
    """Display an image."""
    f = plt.figure(figsize=(fig_size,fig_size))
    ax = f.add_subplot(111)                                                                                                                                                
    ax.imshow(np.clip(np.power(img,1/2.2),0,1), origin='lower')
    ax.set_title(title)
    ax.set_yticks([])
    ax.set_xticks([])
    plt.show()

def display_error(diff, title, fig_size = 20):
    """Display an error image."""
    f = plt.figure(figsize=(fig_size,fig_size))
    ax = f.add_subplot(111)
    im = ax.imshow(diff, origin='lower', vmax=err_vmax)
    divider = make_axes_locatable(ax)
    cax = divider.append_axes("right", size="5%", pad=0.05)
    plt.colorbar(im, cax=cax)
    ax.set_title(title)
    plt.show()

def render(renderer_asset_id, renderer_name, params):
    """Render an image."""
    # Create a film for the output
    film = lm.load_film('film_' + renderer_asset_id, 'bitmap', {
        'w': params['w'],
        'h': params['h']
    })

    # Execute rendering
    renderer = lm.load_renderer(renderer_asset_id, renderer_name, {
        **params,
        'output': film.loc()
    })
    out = renderer.render()
    
    return np.copy(film.buffer()), out

def display_diff_gauss(img1, img2, fig_size = 20, diff_gauss_scale = 10):
    """Apply gaussian blur to the difference between two given images and display."""
    # Compute gaussian blur of the difference
    diff = np.abs(gaussian_filter(img1 - img2, sigma=3))

    # Display image
    f = plt.figure(figsize=(fig_size,fig_size))
    ax = f.add_subplot(111)
    ax.imshow(np.clip(np.power(diff*diff_gauss_scale,1/2.2),0,1), origin='lower')
    ax.set_yticks([])
    ax.set_xticks([])
    plt.show() 

# -------------------------------------------------------------------------------------------------

def normalization(img):
    """Compute normalization factor"""
    w,h,_ = img.shape
    r = img[:,:,0]
    g = img[:,:,1]
    b = img[:,:,2]
    lum = 0.212671 * r + 0.715160 * g + 0.072169 * b
    return np.sum(lum) / (w*h)
