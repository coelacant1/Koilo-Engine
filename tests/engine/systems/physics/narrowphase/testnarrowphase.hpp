// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <unity.h>

class TestGjk {
public:
    static void TestSeparatedSpheres();
    static void TestTouchingSpheres();
    static void TestPenetratingSpheres();
    static void TestSeparatedBoxes();
    static void TestPenetratingBoxes();
    static void RunAllTests();
};

class TestEpa {
public:
    static void TestSpherePenetrationDepthAndNormal();
    static void TestBoxPenetrationOnAxis();
    static void RunAllTests();
};

class TestManifoldGenerator {
public:
    static void TestSphereSphereContact();
    static void TestSphereSphereSeparation();
    static void TestSpherePlaneContact();
    static void TestPlaneSphereContact();
    static void TestSphereBoxContact();
    static void TestBoxBoxPenetration();
    static void TestBoxBoxStackedFourContacts();
    static void TestBoxBoxEdgeEdgeSingleContact();
    static void TestNormalConventionFromBToA();
    static void TestProxyIdCanonicalization();
    static void TestSphereTriangleMeshFace();
    static void TestSphereTriangleMeshSeparated();
    static void TestSphereTriangleMeshEdge();
    static void TestSphereTriangleMeshVertex();
    static void TestMeshSphereReverseDispatch();
    static void TestSphereTriangleMeshRotated();
    static void TestSphereTriangleMeshDegenerateIgnored();
    static void TestCapsuleTriangleMeshFace();
    static void TestCapsuleTriangleMeshSeparated();
    static void TestCapsuleTriangleMeshParallelOverInterior();
    static void TestMeshCapsuleReverseDispatch();
    static void TestBoxTriangleMeshFaceMultiContact();
    static void TestBoxTriangleMeshSeparated();
    static void TestBoxTriangleMeshEdge();
    static void TestMeshBoxReverseDispatch();
    static void TestSphereHeightfieldFlat();
    static void TestSphereHeightfieldSeparated();
    static void TestSphereHeightfieldReverseDispatch();
    static void TestCapsuleHeightfieldFlat();
    static void TestCapsuleHeightfieldSeparated();
    static void TestBoxHeightfieldFlatMultiContact();
    static void TestBoxHeightfieldSeparated();
    static void TestBoxHeightfieldReverseDispatch();
    static void RunAllTests();
};
