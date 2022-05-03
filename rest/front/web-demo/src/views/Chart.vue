<template>
  <v-container fluid>
    <v-sparkline
      :value="get_chart_value"
	  color="red"
	  fill
      smooth="3"
      padding="6"
      stroke-linecap="round"
    ></v-sparkline>
  </v-container>
</template>

<script>
export default {
  data() {
    return {
      timer: null
    };
  },
  computed: {
    get_chart_value() {
      return this.$store.state.chart_value;
    }
  },
  methods: {
    updateData: function() {
      this.$store.dispatch("update_chart_value");
    }
  },
  mounted() {
    clearInterval(this.timer);
    this.timer = setInterval(this.updateData, 1000);
  },
  destroyed: function() {
    clearInterval(this.timer);
  }
};
</script>
