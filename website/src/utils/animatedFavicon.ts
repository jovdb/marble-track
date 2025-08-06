/**
 * AnimatedFavicon - Creates a rotating favicon animation in the browser tab
 * Uses HTML5 Canvas to render rotated versions of an image and updates the favicon dynamically
 */
class AnimatedFavicon {
  private canvas: HTMLCanvasElement;
  private ctx: CanvasRenderingContext2D;
  private img: HTMLImageElement;
  private intervalId: number | null = null;
  private angle = 0;
  private readonly size = 32;
  private readonly rotationSpeed = 0.005; // Radians per frame
  private readonly updateInterval = 32; // 32ms = ~31 FPS

  constructor() {
    this.canvas = document.createElement('canvas');
    this.canvas.width = this.size;
    this.canvas.height = this.size;
    this.ctx = this.canvas.getContext('2d')!;
    this.img = new Image();
  }

  /**
   * Load the image to be used for the favicon animation
   */
  async loadImage(imageSrc: string): Promise<void> {
    return new Promise((resolve, reject) => {
      this.img.onload = () => resolve();
      this.img.onerror = reject;
      this.img.src = imageSrc;
    });
  }

  /**
   * Draw the image rotated by the current angle on the canvas
   */
  private drawRotatedImage(): void {
    // Clear canvas
    this.ctx.clearRect(0, 0, this.size, this.size);
    
    // Save context and apply rotation
    this.ctx.save();
    this.ctx.translate(this.size / 2, this.size / 2);
    this.ctx.rotate(this.angle);
    
    // Draw image centered
    this.ctx.drawImage(
      this.img, 
      -this.size / 2, 
      -this.size / 2, 
      this.size, 
      this.size
    );
    
    this.ctx.restore();
  }

  /**
   * Update the browser's favicon with the current rotated image
   */
  private updateFavicon(): void {
    this.drawRotatedImage();
    
    // Convert canvas to data URL
    const dataURL = this.canvas.toDataURL('image/png');
    
    // Find or create favicon link element
    let link = document.querySelector('link[rel="shortcut icon"]') as HTMLLinkElement;
    if (!link) {
      link = document.createElement('link');
      link.rel = 'shortcut icon';
      link.type = 'image/png';
      document.head.appendChild(link);
    }
    
    // Update favicon
    link.href = dataURL;
  }

  /**
   * Animation loop - increments rotation angle and updates favicon
   */
  private animate = (): void => {
    this.angle += this.rotationSpeed;
    if (this.angle >= 2 * Math.PI) {
      this.angle = 0;
    }
    
    this.updateFavicon();
  };

  /**
   * Start the favicon rotation animation
   */
  async start(imageSrc: string): Promise<void> {
    try {
      await this.loadImage(imageSrc);
      this.intervalId = window.setInterval(this.animate, this.updateInterval);
    } catch (error) {
      console.error('Failed to load favicon image:', error);
    }
  }

  /**
   * Stop the favicon animation and clean up resources
   */
  stop(): void {
    if (this.intervalId) {
      clearInterval(this.intervalId);
      this.intervalId = null;
    }
  }
}

export default AnimatedFavicon;
