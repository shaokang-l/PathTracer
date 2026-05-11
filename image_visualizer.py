import numpy as np
import matplotlib.pyplot as plt
from skimage import io
from skimage.exposure import histogram
import os

def load_image(image_path):
    # Load and preprocess the image
    image = io.imread(image_path)
    # Ensures pixel values are between 0 and 1
    image = img_as_float(image).astype(np.float64)
    # Read as RGB, not RGBA
    if image.shape[2] > 3:
        image = image[:, :, :3]
    print("Image loaded and normalized.")
    return image

def plot_rgb_histogram(image_path, bins=256, figure_size=(10, 6)):
    """
    Plot RGB histogram for an input image using skimage.
    
    Parameters:
    image_path (str): Path to the image file
    bins (int): Number of bins for the histogram
    figure_size (tuple): Size of the output figure (width, height)
    """
    # Read image using skimage
    img = io.imread(image_path)
    
    # Create figure and axis
    plt.figure(figsize=figure_size)
    
    # Plot histograms for each channel
    colors = ('red', 'green', 'blue')
    for i, color in enumerate(colors):
        # Calculate histogram using skimage's histogram function
        hist, bin_centers = histogram(img[:,:,i], nbins=bins, normalize=True)
        
        # Apply light smoothing using moving average
        smoothed_hist = np.convolve(hist, np.ones(5)/5, mode='valid')
        smoothed_bins = bin_centers[2:-2]  # Adjust bins to match smoothed histogram length
        
        plt.plot(smoothed_bins, smoothed_hist, color=color, alpha=0.7, 
                linewidth=2, label=color.upper())
    
    # Customize plot
    plt.xlabel('Pixel Intensity')
    plt.ylabel('Normalized Frequency')
    plt.title('RGB Color Histogram')
    plt.legend()
    plt.grid(True, alpha=0.2)
    
    # Remove top and right spines
    plt.gca().spines['top'].set_visible(False)
    plt.gca().spines['right'].set_visible(False)
    
    plt.show()

def plot_rgb_histograms_comparison(image1_path, image2_path, bins=256, figure_size=(15, 5)):
    """
    Plot RGB histograms for two images side by side for comparison.
    
    Parameters:
    image1_path (str): Path to the first image
    image2_path (str): Path to the second image
    bins (int): Number of bins for the histogram
    figure_size (tuple): Size of the output figure (width, height)
    """
    # Read both images
    img1 = io.imread(image1_path)
    img2 = io.imread(image2_path)
    
    # Create subplots
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=figure_size)
    
    # Plot histograms for each image
    for ax, img, title in zip([ax1, ax2], [img1, img2], ['Image 1', 'Image 2']):
        colors = ('red', 'green', 'blue')
        for i, color in enumerate(colors):
            hist, bin_centers = histogram(img[:,:,i], nbins=bins, normalize=True)
            
            # Apply smoothing
            smoothed_hist = np.convolve(hist, np.ones(5)/5, mode='valid')
            smoothed_bins = bin_centers[2:-2]
            
            ax.plot(smoothed_bins, smoothed_hist, color=color, alpha=0.7, 
                   linewidth=2, label=color.upper())
        
        ax.set_xlabel('Pixel Intensity')
        ax.set_ylabel('Normalized Frequency')
        ax.set_title(f'RGB Histogram - {title}')
        ax.legend()
        ax.grid(True, alpha=0.2)
        ax.spines['top'].set_visible(False)
        ax.spines['right'].set_visible(False)
    
    plt.tight_layout()
    plt.show()

def plot_rgb_histograms_comparison(image1_path, image2_path, image3_path, image4_path, bins=256, figure_size=(20, 8)):
    # Read all images
    img1 = io.imread(image1_path)
    img2 = io.imread(image2_path)
    img3 = io.imread(image3_path)
    img4 = io.imread(image4_path)

    # Create subplots - 2 rows for image and histogram
    fig, ((ax_img1, ax_img2, ax_img3, ax_img4), 
          (ax_hist1, ax_hist2, ax_hist3, ax_hist4)) = plt.subplots(2, 4, figsize=figure_size)

    # Plot images and histograms
    for ax_img, ax_hist, img, title in zip(
        [ax_img1, ax_img2, ax_img3, ax_img4],
        [ax_hist1, ax_hist2, ax_hist3, ax_hist4],
        [img1, img2, img3, img4],
        ['Original', 'OT', 'Linear', 'Var']):
        
        # Show image
        ax_img.imshow(img)
        ax_img.set_title(f'Image - {title}')
        ax_img.axis('off')  # Hide axes for image
        
        # Plot histogram
        colors = ('red', 'green', 'blue')
        for i, color in enumerate(colors):
            hist, bin_centers = histogram(img[:,:,i], nbins=bins, normalize=True)
            
            # Apply smoothing
            smoothed_hist = np.convolve(hist, np.ones(5)/5, mode='valid')
            smoothed_bins = bin_centers[2:-2]
            
            ax_hist.plot(smoothed_bins, smoothed_hist, color=color, alpha=0.7, 
                        linewidth=2, label=color.upper())
        
        ax_hist.set_xlabel('Pixel Intensity')
        ax_hist.set_ylabel('Normalized Frequency')
        ax_hist.set_title(f'RGB Histogram - {title}')
        ax_hist.legend()
        ax_hist.grid(True, alpha=0.2)
        ax_hist.spines['top'].set_visible(False)
        ax_hist.spines['right'].set_visible(False)

    plt.tight_layout()
    plt.show()


from skimage.exposure import histogram
from skimage.util import compare_images

def compute_mse(image1_path,image2_path):
    """
    Compute Mean Squared Error (MSE) between two images.
    
    Parameters:
    image1_path (str): Path to the first image
    image2_path (str): Path to the second image
    
    Returns:
    float: Mean Squared Error between the two images
    """
    # Read images
    img1 = io.imread(image1_path)
    img2 = io.imread(image2_path)
    
    # Ensure images have same shape
    if img1.shape != img2.shape:
        raise ValueError("Images must have the same dimensions")
    
    # Calculate MSE
    mse = np.mean((img1 - img2) ** 2)
    
    return mse
    

def plot_image_difference(image1_path, image2_path, figure_size=(15, 10)):
    """
    Visualize the difference between two images, showing:
    1. Original images
    2. Difference image
    3. RGB histograms of the difference
    
    Parameters:
    image1_path (str): Path to the first image
    image2_path (str): Path to the second image
    figure_size (tuple): Size of the output figure (width, height)
    """
    # Read images
    img1 = io.imread(image1_path)
    img2 = io.imread(image2_path)
    
    # Ensure images have same shape
    if img1.shape != img2.shape:
        raise ValueError("Images must have the same dimensions")
    
    # Calculate difference
    diff = compare_images(img1, img2, method='diff')
    
    # Create figure with subplots
    fig = plt.figure(figsize=figure_size)
    gs = fig.add_gridspec(2, 3)
    
    # Plot original images and difference
    ax1 = fig.add_subplot(gs[0, 0])
    ax2 = fig.add_subplot(gs[0, 1])
    ax3 = fig.add_subplot(gs[0, 2])
    ax4 = fig.add_subplot(gs[1, :])
    
    # Display images
    ax1.imshow(img1)
    ax1.set_title('Image 1')
    ax1.axis('off')
    
    ax2.imshow(img2)
    ax2.set_title('Image 2')
    ax2.axis('off')
    
    # Display difference image
    diff_img = ax3.imshow(diff, cmap='RdBu_r')
    ax3.set_title('Difference')
    ax3.axis('off')
    plt.colorbar(diff_img, ax=ax3)

    # Calculate MSE
    mse = np.mean((img1 - img2) ** 2)
    print(f"Mean Squared Error (MSE): {mse:.4f}")
    
    # Plot histogram of differences for each channel
    colors = ('red', 'green', 'blue')
    for i, color in enumerate(colors):
        # Calculate histogram of difference
        hist, bin_centers = histogram(diff[:,:,i], nbins=256, normalize=True)
        
        # Apply smoothing
        smoothed_hist = np.convolve(hist, np.ones(5)/5, mode='valid')
        smoothed_bins = bin_centers[2:-2]
        
        ax4.plot(smoothed_bins, smoothed_hist, color=color, alpha=0.7,
                linewidth=2, label=color.upper())
    
    # Customize histogram plot
    ax4.set_xlabel('Difference Intensity')
    ax4.set_ylabel('Normalized Frequency')
    ax4.set_title('RGB Histogram of Differences')
    ax4.legend()
    ax4.grid(True, alpha=0.2)
    ax4.spines['top'].set_visible(False)
    ax4.spines['right'].set_visible(False)
    
    plt.tight_layout()
    plt.show()

def analyze_image_differences(image1_path, image2_path):
    """
    Analyze and print statistical measures of differences between two images.
    
    Parameters:
    image1_path (str): Path to the first image
    image2_path (str): Path to the second image
    """
    # Read images
    img1 = io.imread(image1_path)
    img2 = io.imread(image2_path)
    
    # Calculate difference
    diff = compare_images(img1, img2, method='diff')
    
    # Calculate statistics for each channel
    stats = {}
    for i, channel in enumerate(['Red', 'Green', 'Blue']):
        channel_diff = diff[:,:,i]
        stats[channel] = {
            'mean': np.mean(channel_diff),
            'std': np.std(channel_diff),
            'max': np.max(channel_diff),
            'min': np.min(channel_diff),
            'median': np.median(channel_diff)
        }
    
    # Print statistics
    print("Image Difference Statistics:")
    print("-" * 50)
    for channel, measures in stats.items():
        print(f"\n{channel} Channel:")
        for measure, value in measures.items():
            print(f"{measure.capitalize():>10}: {value:.4f}")

def analyze_gaussian_input(gaussian_image):
    plt.figure(figsize=(15, 5))
    
    # Plot histograms for each channel
    channels = ['Red', 'Green', 'Blue']
    for i in range(3):
        plt.subplot(1, 3, i+1)
        plt.hist(gaussian_image[:,:,i].ravel(), bins=50, density=True)
        plt.title(f'{channels[i]} Channel Distribution')
        plt.xlabel('Value')
        plt.ylabel('Density')
    
    # Print statistics
    print("Gaussian Image Statistics:")
    print("Mean:", gaussian_image.mean(axis=(0,1)))
    print("Std:", gaussian_image.std(axis=(0,1)))
    print("Min:", gaussian_image.min(axis=(0,1)))
    print("Max:", gaussian_image.max(axis=(0,1)))
    
    plt.tight_layout()
    plt.show()

def plot_2d_distributions(gaussian_image):
    fig, axes = plt.subplots(1, 3, figsize=(15, 5))
    
    # R-G distribution
    axes[0].scatter(gaussian_image[:,:,0].ravel(), 
                   gaussian_image[:,:,1].ravel(), 
                   alpha=0.1, s=1)
    axes[0].set_title('R-G Distribution')
    axes[0].set_xlabel('Red')
    axes[0].set_ylabel('Green')
    
    # R-B distribution
    axes[1].scatter(gaussian_image[:,:,0].ravel(), 
                   gaussian_image[:,:,2].ravel(), 
                   alpha=0.1, s=1)
    axes[1].set_title('R-B Distribution')
    axes[1].set_xlabel('Red')
    axes[1].set_ylabel('Blue')
    
    # G-B distribution
    axes[2].scatter(gaussian_image[:,:,1].ravel(), 
                   gaussian_image[:,:,2].ravel(), 
                   alpha=0.1, s=1)
    axes[2].set_title('G-B Distribution')
    axes[2].set_xlabel('Green')
    axes[2].set_ylabel('Blue')
    
    plt.tight_layout()
    plt.show()


# Example usage
def main():
    print(compute_mse("./ap.png","./gt.png"))
    # print(compute_mse("./cos.png","./gt.png"))
    # print(compute_mse("./uniform.png","./gt.png"))
    # if os.path.exists('./part-1.png'):
    #     plot_image_difference('./ref_blank/hw7_1.png', './part-1.png')
    # if os.path.exists('./part-2.png'):
    #     plot_image_difference('./ref_blank/hw7_2.png', './part-2.png')
    #if there exist a png named bonus.png, it will be compared with the reference bonus.png
    # if os.path.exists('./bonus.png'):
    #     plot_image_difference('./ref_blank/bonus.png', './bonus.png')
    

if __name__ == "__main__":
    main()