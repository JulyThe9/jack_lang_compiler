from anytree import Node, RenderTree
from anytree.exporter import DotExporter
import re
import sys

def parse_ast(file_path):
    nodes = {}
    
    with open(file_path, 'r') as file:
        content = file.read()
    
    # Split the input into node sections
    node_sections = re.split(r'\n\n(?=AstNode #)', content.strip())
    
    for section in node_sections:
        if not section.startswith('AstNode'):
            continue
            
        # Extract node ID
        id_match = re.search(r'AstNode #(\d+)', section)
        if not id_match:
            continue
        node_id = int(id_match.group(1))
        
        # Extract node type
        type_match = re.search(r'Type: (\w+)', section)
        node_type = type_match.group(1) if type_match else "UNKNOWN"
        
        # Extract node value (if exists)
        val_match = re.search(r'Val: ([\w\.]+|None)', section)
        node_val = val_match.group(1) if val_match and val_match.group(1) != "None" else None
        
        # Create node label
        label = f"#{node_id} {node_type}"
        if node_val:
            label += f" ({node_val})"
        
        # Create the node if it doesn't exist
        if node_id not in nodes:
            nodes[node_id] = Node(label)
        else:
            # Update existing node if it was a placeholder
            nodes[node_id].name = label
        
        # Extract all children IDs
        children_part = re.search(r'Children:(.*)', section)
        if children_part:
            children_ids = [int(id) for id in re.findall(r'#(\d+)', children_part.group(1))]
            for child_id in children_ids:
                if child_id not in nodes:
                    # Create placeholder node
                    nodes[child_id] = Node(f"#{child_id} [placeholder]", parent=nodes[node_id])
                else:
                    # Set parent if not already set
                    if nodes[child_id].parent is None:
                        nodes[child_id].parent = nodes[node_id]
    
    return nodes.get(0, None)  # Return root node

def visualize_ast(input_file, output_image=None):
    root = parse_ast(input_file)
    
    if not root:
        print("Failed to parse AST")
        return
    
    # Print text representation
    print("Text Representation:")
    print("====================")
    for pre, _, node in RenderTree(root):
        print(f"{pre}{node.name}")
    
    # Generate image if requested
    if output_image:
        try:
            DotExporter(root,
                      nodenamefunc=lambda node: node.name,
                      nodeattrfunc=lambda node: "shape=box, style=filled, fillcolor=lightblue"
                      ).to_picture(output_image)
            print(f"\nVisualization saved to {output_image}")
        except Exception as e:
            print(f"\nFailed to generate image: {e}")
            print("Make sure you have Graphviz installed (https://graphviz.org/download/)")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python ast_visualizer.py <input_file> [output_image]")
        print("Example: python ast_visualizer.py ast_input.txt ast_tree.png")
        exit(1)
    
    input_file = sys.argv[1]
    output_image = sys.argv[2] if len(sys.argv) > 2 else None
    
    visualize_ast(input_file, output_image)