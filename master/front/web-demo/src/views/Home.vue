<template>
  <v-container>
    <v-layout text-xs-center-wrap>
      <v-flex xs12 sm6>
        <apexcharts
          width="500"
          height="350"
          type="area"
          :options="chartOptions"
          :series="series"
        ></apexcharts>
      </v-flex>
      <v-flex xs12 sm6>
        <v-btn class="ma-2" outline color="green">POWER</v-btn>
      </v-flex>
    </v-layout>
  </v-container>
</template>

<script>
import VueApexCharts from "vue-apexcharts";

export default {
  name: "Chart",
  components: {
    apexcharts: VueApexCharts,
  },
  data: function () {
    return {
      chartOptions: {
        chart: {
          id: "basic-bar",
        },
        xaxis: {
          categories: [1991, 1992, 1993, 1994, 1995, 1996, 1997, 1998],
        },
      },
      series: [
        {
          name: "series-1",
          data: [30, 40, 45, 50, 49, 60, 70, 91],
        },
      ],
    };
  },
};
</script>
