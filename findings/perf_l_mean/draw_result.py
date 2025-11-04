import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import sys

def plot_performance_data(csv_file):
    # Read the CSV data
    try:
        # Try to read with header
        df = pd.read_csv(csv_file)
    except pd.errors.ParserError:
        # If error, try reading without header and set column names manually
        df = pd.read_csv(csv_file, header=None, names=['l_value', 'msg_rate', 'bandwidth_gbits'])
    
    # Create a figure with a single subplot
    fig, ax = plt.subplots(figsize=(10, 6))
    fig.suptitle('Network Performance Metrics vs L_Value', fontsize=16)
    
    # Set the style
    sns.set_style("whitegrid")
    
    # Plot message rate
    sns.lineplot(x='l_value', y='msg_rate', data=df, marker='o', linewidth=2, ax=ax, color='blue', label='Message Rate')
    
    ax.set_xlabel('L Value', fontsize=12)
    ax.set_ylabel('Value', fontsize=12)
    ax.set_title('Message Rate', fontsize=14)
    ax.grid(True)
    ax.legend()
    
    # Add value annotations to each point
    for i, row in df.iterrows():
        ax.annotate(f"{row['msg_rate']:.2f}", 
                     (row['l_value'], row['msg_rate']),
                     textcoords="offset points", 
                     xytext=(0,10), 
                     ha='center', color='blue')
    
    # Adjust layout and save the figure
    plt.tight_layout()
    plt.subplots_adjust(top=0.9)
    
    # Save the figure
    output_file = 'performance_metrics.png'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"Figure saved as {output_file}")
    
    # Display the figure
    plt.show()

if __name__ == "__main__":
    # Get the CSV file from command line arguments or use default
    if len(sys.argv) > 1:
        csv_file = sys.argv[1]
    else:
        # Default to input.csv if no file specified
        csv_file = 'ib_write_benchmark_results.csv'
    
    plot_performance_data(csv_file)