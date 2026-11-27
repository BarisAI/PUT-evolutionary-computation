import math
import unittest
from analyze import edges_of, nodes_of, mean_overlap, overlaps, pearson

class SimilarityTests(unittest.TestCase):
    def test_edges_undirected_rotation_reversal(self):
        a=[0,1,2,3]
        self.assertEqual(edges_of(a), edges_of([2,3,0,1]))
        self.assertEqual(edges_of(a), edges_of([3,2,1,0]))
        self.assertEqual(len(edges_of(a)), 4)
        self.assertEqual(len(edges_of(a) & edges_of([0,2,1,3])), 2)

    def test_selected_nodes_ignore_order(self):
        self.assertEqual(nodes_of([2,1,3]), nodes_of([1,3,2]))
        self.assertEqual(len(nodes_of([0,1,2]) & nodes_of([1,3,4])), 1)

    def test_mean_excludes_self_even_with_duplicate_solutions(self):
        s=[frozenset((1,2)),frozenset((2,3)),frozenset((1,2))]
        self.assertEqual(mean_overlap(s),[1.5,1.0,1.5])

    def test_self_exclusion_for_good_sample(self):
        sets=[nodes_of([1,2]),nodes_of([2,3]),nodes_of([1,2])]
        best_idx=0
        xs=[overlap for i,overlap in enumerate(overlaps(sets,sets[best_idx])) if i!=best_idx]
        self.assertEqual(xs,[1,2])

    def test_pearson_sign(self):
        self.assertTrue(math.isclose(pearson([1,2,3], [3,2,1]), -1))
        self.assertTrue(math.isclose(pearson([1,2,3], [1,2,3]), 1))

if __name__=='__main__': unittest.main()
