"""
GDB Pretty Printers for DynamicArray and GameHashMap
Usage: source gdb_printers.py in GDB or add to .gdbinit
"""

import gdb
import gdb.printing


class DynamicArrayPrinter:
    """Pretty printer for DynamicArray<T, Allocator>"""
    
    def __init__(self, val):
        self.val = val
        
    def to_string(self):
        size = int(self.val['m_Size'])
        capacity = int(self.val['m_Capacity'])
        return f"DynamicArray of size {size}, capacity {capacity}"
    
    def children(self):
        size = int(self.val['m_Size'])
        data = self.val['m_Data']
        
        # Yield metadata
        yield '[size]', self.val['m_Size']
        yield '[capacity]', self.val['m_Capacity']
        yield '[empty]', size == 0
        
        # Yield array elements
        for i in range(size):
            yield f'[{i}]', data[i]
    
    def display_hint(self):
        return 'array'


class GameHashMapPrinter:
    """Pretty printer for Plu::GameHashMap<TKey, TValue, THash, TAllocator>"""
    
    def __init__(self, val):
        self.val = val
        
    def to_string(self):
        size = int(self.val['size_'])
        capacity = int(self.val['capacity_'])
        tombstones = int(self.val['tombstones_'])
        load_factor = float(size) / float(capacity) if capacity > 0 else 0.0
        return f"GameHashMap of size {size}, capacity {capacity}, load {load_factor:.2f}"
    
    def children(self):
        size = int(self.val['size_'])
        capacity = int(self.val['capacity_'])
        tombstones = int(self.val['tombstones_'])
        buckets = self.val['buckets_']
        
        # Yield metadata
        yield '[size]', self.val['size_']
        yield '[capacity]', self.val['capacity_']
        yield '[tombstones]', self.val['tombstones_']
        
        if capacity > 0:
            load_factor = float(size) / float(capacity)
            yield '[load_factor]', f"{load_factor:.3f}"
        
        # Yield key-value pairs
        count = 0
        for i in range(capacity):
            if count >= size:
                break
                
            bucket = buckets[i]
            dib = int(bucket['dib'])
            occupied = bool(bucket['occupied'])
            
            # Skip empty buckets (dib == 0) and tombstones (dib == 255)
            if occupied and dib != 0 and dib != 255:
                # Cast storage to std::pair pointer
                storage_ptr = bucket['storage'].cast(
                    gdb.lookup_type('unsigned char').pointer()
                )
                
                # Get the template parameters for key and value types
                # This is a bit tricky in GDB
                try:
                    # Try to get the ValueType from the HashMap type
                    value_type = self.val.type.template_argument(0)
                    mapped_type = self.val.type.template_argument(1)
                    
                    # Create the pair type
                    pair_type = gdb.lookup_type(
                        f'std::pair<{value_type.name} const, {mapped_type.name}>'
                    )
                    
                    # Cast and dereference
                    pair_ptr = storage_ptr.cast(pair_type.pointer())
                    pair = pair_ptr.dereference()
                    
                    key = pair['first']
                    value = pair['second']
                    
                    yield f'[{key}]', value
                    count += 1
                except:
                    # Fallback if type parsing fails
                    yield f'[bucket_{i}]', f'DIB={dib} (occupied)'
                    count += 1
    
    def display_hint(self):
        return 'map'


class GameHashMapBucketPrinter:
    """Pretty printer for GameHashMap::Bucket (for internal debugging)"""
    
    def __init__(self, val):
        self.val = val
        
    def to_string(self):
        dib = int(self.val['dib'])
        occupied = bool(self.val['occupied'])
        
        if dib == 0:
            return "[Empty Bucket]"
        elif dib == 255:
            return "[Tombstone]"
        elif occupied:
            return f"[Occupied Bucket, DIB={dib}]"
        else:
            return f"[Invalid Bucket State, DIB={dib}]"


def build_pretty_printer():
    """Build and return the pretty printer object"""
    pp = gdb.printing.RegexpCollectionPrettyPrinter("CustomContainers")
    
    # Register DynamicArray printer
    pp.add_printer('DynamicArray', '^DynamicArray<.*>$', DynamicArrayPrinter)
    
    # Register GameHashMap printer
    pp.add_printer('GameHashMap', '^Plu::GameHashMap<.*>$', GameHashMapPrinter)
    
    # Register Bucket printer (optional, for deep debugging)
    pp.add_printer('GameHashMapBucket', 
                   '^Plu::GameHashMap<.*>::Bucket$', 
                   GameHashMapBucketPrinter)
    
    return pp


# Register the pretty printer with GDB
gdb.printing.register_pretty_printer(
    gdb.current_objfile(),
    build_pretty_printer(),
    replace=True
)

print("Custom container pretty printers loaded!")
print("  - DynamicArray<T, Allocator>")
print("  - Plu::GameHashMap<TKey, TValue, THash, TAllocator>")
print("  - Plu::GameHashMap<...>::Bucket")
