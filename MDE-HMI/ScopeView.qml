// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtCharts

//![1]
ChartView {
    id: chartView
    animationOptions: ChartView.NoAnimation
    margins {
        top: 0
        bottom: 0
        left: 0
        right: 0
    }
    anchors {
        fill: parent
        margins: 0
    }
    backgroundRoundness: 0
    plotAreaColor: "transparent"
    backgroundColor: "transparent"
    titleColor: "red"


    

    legend {
        visible: true
        alignment: Qt.AlignBottom
        labelColor: "white"
        borderColor: "white"
        font.bold: true
        font.pixelSize: 14
        markerShape: Legend.MarkerShapeFromSeries
    }

    property bool openGL: openGLSupported
    onOpenGLChanged: {
        if (openGLSupported) {
            var series1 = series("signal 1")
            if (series1)
                series1.useOpenGL = openGL;
            var series2 = series("signal 2")
            if (series2)
                series2.useOpenGL = openGL;
        }
    }

    ValueAxis {
        titleText: "<font color='white'>Voltage (V)</font>" // not sure if there's a better way
        id: axisY1
        min: -200
        max: 200
        labelsColor: "white"
        labelsFont.bold: true
        titleFont.bold: true
        visible: true
    }

    ValueAxis {
        titleText: "<font color='white'>Current (A)</font>"
        id: axisY2
        min: -200
        max: 200
        labelsColor: "white"
        labelsFont.bold: true
        titleFont.bold: true
    }

    ValueAxis {
        titleText: "<font color='white'>Index</font>"
        id: axisX
        min: 0
        max: 100
        labelsColor: "white"
        labelsFont.bold: true
    }

    LineSeries {
        id: lineSeries1
        name: "AC Voltage"
        axisX: axisX
        axisY: axisY1
        useOpenGL: chartView.openGL
        color: "orange"
        width: 5
    }
    LineSeries {
        id: lineSeries2
        name: "AC Current"
        axisX: axisX
        axisYRight: axisY2
        useOpenGL: chartView.openGL
        color: "green"
        width: 5
    }
//![1]

    //![2]
    Timer {
        id: refreshTimer
        interval: 1 / 60 * 1000 // 60 Hz
        running: true
        repeat: true
        onTriggered: {
            dataSource.update(chartView.series(0));
            dataSource.update(chartView.series(1));
            axisX["max"] = Math.max(dataSource.getLastXPos(), 100);
            axisX["min"] = Math.max(0, dataSource.getLastXPos() - 100);
        }
    }
    //![2]

    //![3]
    function changeSeriesType(type) {
        chartView.removeAllSeries();

        // Create two new series of the correct type. Axis x is the same for both of the series,
        // but the series have their own y-axes to make it possible to control the y-offset
        // of the "signal sources".
        var series1
        var series2
        if (type === "line") {
            series1 = chartView.createSeries(ChartView.SeriesTypeLine, "signal 1",
                                             axisX, axisY1);
            series1.useOpenGL = chartView.openGL

            series2 = chartView.createSeries(ChartView.SeriesTypeLine, "signal 2",
                                             axisX, axisY2);
            series2.useOpenGL = chartView.openGL
        } else {
            series1 = chartView.createSeries(ChartView.SeriesTypeScatter, "signal 1",
                                             axisX, axisY1);
            series1.markerSize = 2;
            series1.borderColor = "transparent";
            series1.useOpenGL = chartView.openGL

            series2 = chartView.createSeries(ChartView.SeriesTypeScatter, "signal 2",
                                             axisX, axisY2);
            series2.markerSize = 2;
            series2.borderColor = "transparent";
            series2.useOpenGL = chartView.openGL
        }
    }

    function createAxis(min, max) {
        // The following creates a ValueAxis object that can be then set as a x or y axis for a series
        return Qt.createQmlObject("import QtQuick 2.0; import QtCharts 2.0; ValueAxis { min: "
                                  + min + "; max: " + max + " }", chartView);
    }
    //![3]

    function setAnimations(enabled) {
        if (enabled)
            chartView.animationOptions = ChartView.SeriesAnimations;
        else
            chartView.animationOptions = ChartView.NoAnimation;
    }

    function changeRefreshRate(rate) {
        refreshTimer.interval = 1 / Number(rate) * 1000;
    }
}
