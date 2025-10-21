// Custom JavaScript for TLE92466ED driver documentation

// Initialize when DOM is loaded
document.addEventListener('DOMContentLoaded', function() {
  // Add copy buttons to code blocks if not already present
  addCopyButtonsToCodeBlocks();
  
  // Enhance register field displays
  enhanceRegisterFields();
  
  // Add tooltips to specification values
  addSpecificationTooltips();
});

/**
 * Add copy buttons to code blocks
 */
function addCopyButtonsToCodeBlocks() {
  const codeBlocks = document.querySelectorAll('pre code');
  
  codeBlocks.forEach(block => {
    if (block.parentElement.querySelector('.copy-button')) {
      return; // Already has a copy button
    }
    
    const button = document.createElement('button');
    button.className = 'copy-button';
    button.textContent = 'Copy';
    button.onclick = function() {
      navigator.clipboard.writeText(block.textContent).then(() => {
        button.textContent = 'Copied!';
        setTimeout(() => {
          button.textContent = 'Copy';
        }, 2000);
      });
    };
    
    block.parentElement.appendChild(button);
  });
}

/**
 * Enhance register field displays with hover effects
 */
function enhanceRegisterFields() {
  const registerFields = document.querySelectorAll('.register-field');
  
  registerFields.forEach(field => {
    field.addEventListener('mouseenter', function() {
      this.style.backgroundColor = 'rgba(0, 0, 0, 0.15)';
    });
    
    field.addEventListener('mouseleave', function() {
      this.style.backgroundColor = 'rgba(0, 0, 0, 0.05)';
    });
  });
}

/**
 * Add tooltips to specification values
 */
function addSpecificationTooltips() {
  // This can be extended to add interactive tooltips
  // for technical specifications throughout the documentation
}

/**
 * Smooth scroll to anchors
 */
document.querySelectorAll('a[href^="#"]').forEach(anchor => {
  anchor.addEventListener('click', function (e) {
    e.preventDefault();
    const target = document.querySelector(this.getAttribute('href'));
    if (target) {
      target.scrollIntoView({
        behavior: 'smooth',
        block: 'start'
      });
    }
  });
});
