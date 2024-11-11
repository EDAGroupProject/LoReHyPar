import hypernetx as hnx
import matplotlib.pyplot as plt
import os

# 读取 design.net 文件，构建超边
def read_design_net(file_path):
    hyperedges = {}
    with open(file_path, 'r') as file:
        for line in file:
            parts = line.strip().split()
            if len(parts) < 3:
                continue  # 跳过无效行

            source_node = parts[0]
            weight = float(parts[1])
            drain_nodes = parts[2:]
            edge_name = f"edge"  # 每个超边的名称

            # 将超边记录为: {edge_name: [source_node, *drain_nodes]}
            hyperedges[edge_name] = {"nodes": [source_node] + drain_nodes, "weight": weight}

    return hyperedges

# 读取 design.fpga.out 文件，构建 FPGA 超边
def read_fpga_out(file_path):
    fpgas = {}
    with open(file_path, 'r') as file:
        for line_num, line in enumerate(file, start=1):
            parts = line.strip().split()
            if len(parts) < 2:
                continue  # 跳过无效行

            fpga_name = f"fpga{line_num}"
            nodes = [node.replace("*", "") for node in parts[1:]]  # 移除 "*" 标记
            fpgas[fpga_name] = nodes

    return fpgas

# 构建 HyperNetX 超图
def build_hypergraph(design_net_path, fpga_out_path):
    hyperedges = read_design_net(design_net_path)
    fpgas = read_fpga_out(fpga_out_path)

    # 将 design.net 的超边添加到 HyperNetX 超图
    hnx_hyperedges = {}
    for edge_name, edge_data in hyperedges.items():
        hnx_hyperedges[edge_name] = edge_data["nodes"]

    # 将 FPGA 的超边添加到 HyperNetX 超图
    for fpga_name, nodes in fpgas.items():
        hnx_hyperedges[fpga_name] = nodes

    # 创建 HyperNetX 超图
    H = hnx.Hypergraph(hnx_hyperedges)
    return H

# 可视化并保存超图
def visualize_hypergraph(H, output_folder='output', output_filename='hypergraph5.png'):
    # 创建输出文件夹
    if not os.path.exists(output_folder):
        os.makedirs(output_folder)

    # 绘制超图
    plt.figure(figsize=(10, 8))
    hnx.draw(H, with_node_labels=True, with_edge_labels=True)
    plt.title("Hypergraph Visualization")
    
    # 保存图像
    output_path = os.path.join(output_folder, output_filename)
    plt.savefig(output_path)
    plt.close()
    print(f"Hypergraph saved to {output_path}")

# 指定文件路径
design_net_path = '/home/ubuntu/LoReHyPar/testcase/case01/design.net'
fpga_out_path = '/home/ubuntu/LoReHyPar/testcase/case01/design.fpga.out1'

# 构建超图
H = build_hypergraph(design_net_path, fpga_out_path)

# 可视化并保存图像
visualize_hypergraph(H)
