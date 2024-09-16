import sqlite3
import numpy as np
from sklearn.cluster import KMeans

def run_ml_analysis():
    # Connect to the SQLite database
    conn = sqlite3.connect('network_scanner.db')
    cursor = conn.cursor()

    # Query: Get the port data from the database
    cursor.execute("SELECT port FROM port_scans")
    port_data = cursor.fetchall()

    # Convert the data to a NumPy array
    port_numbers = np.array([port[0] for port in port_data]).reshape(-1, 1)

    # Check if we have enough data for clustering
    if len(port_numbers) > 1:
        # Apply KMeans clustering to the port numbers
        kmeans = KMeans(n_clusters=3)  # You can change the number of clusters
        kmeans.fit(port_numbers)

        # Output the cluster centers (most common port groups)
        print("Cluster Centers (Common Port Groups):", kmeans.cluster_centers_)
    else:
        print("Not enough data for machine learning analysis.")

    conn.close()

if __name__ == "__main__":
    run_ml_analysis()